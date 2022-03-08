import numpy as np
import os
import time

import torch
import torch.nn as nn
import torch.optim as optim
import torch.optim.lr_scheduler as LS
from torch.autograd import Variable

from dataset import get_loader
from evaluate import run_eval
from train_options import parser
from util import get_models, init_lstm, set_train, set_eval
from util import prepare_inputs, forward_ctx
import warnings
warnings.filterwarnings("ignore")

import p2p_networks

args = parser.parse_args()
print(args)

device = torch.device('cuda' if torch.cuda.is_available() else 'cpu') 

############### Data ###############
train_loader = get_loader(
  is_train=True,
  root=args.train, mv_dir=args.train_mv, 
  args=args
)

def get_eval_loaders():
  # We can extend this dict to evaluate on multiple datasets.
  eval_loaders = {
    'TVL': get_loader(
        is_train=False,
        root=args.eval, mv_dir=args.eval_mv,
        args=args),
  }
  return eval_loaders



############### Model ###############
encoder, binarizer, decoder, unet = get_models(
  args=args, v_compress=args.v_compress, 
  bits=args.bits,
  encoder_fuse_level=args.encoder_fuse_level,
  decoder_fuse_level=args.decoder_fuse_level)

netD = p2p_networks.define_D(input_nc=3, ndf=64, netD='n_layers', n_layers_D=4, gpu_ids=[0]) #3+3
#                                opt.n_layers_D, opt.norm, opt.init_type, opt.init_gain, self.gpu_ids)
criterionGAN = p2p_networks.GANLoss(gan_mode='lsgan').to(device)
#print(netD)

nets = [encoder, binarizer, decoder]
if unet is not None:
  nets.append(unet)

gpus = [int(gpu) for gpu in args.gpus.split(',')]
if len(gpus) > 1:
  print("Using GPUs {}.".format(gpus))
  for net in nets:
    net = nn.DataParallel(net, device_ids=gpus)

params = [{'params': net.parameters()} for net in nets]

solver = optim.Adam(
    params,
    lr=args.lr)
solverD = optim.Adam(netD.parameters(),lr=args.lr)

milestones = [int(s) for s in args.schedule.split(',')]
scheduler = LS.MultiStepLR(solver, milestones=milestones, gamma=args.gamma)
schedulerD = LS.MultiStepLR(solverD, milestones=milestones, gamma=args.gamma)

if not os.path.exists(args.model_dir):
  print("Creating directory %s." % args.model_dir)
  os.makedirs(args.model_dir)

############### Checkpoints ###############
def resume(load_model_name, level=0, index):
  names = ['encoder', 'binarizer', 'decoder', 'unet', 'discriminator']
  for net_idx, net in enumerate(nets+[netD]):
    if net is not None:
      name = names[net_idx]
      checkpoint_path = '{}/{}_{}_{:08d}_level{}.pth'.format(
          args.model_dir, load_model_name, 
          name, index, level)

      print('Loading %s from %s...' % (name, checkpoint_path))
      net.load_state_dict(torch.load(checkpoint_path))


def save(index, level=0):
  names = ['encoder', 'binarizer', 'decoder', 'unet', 'discriminator']
  for net_idx, net in enumerate(nets+[netD]):
    if net is not None:
      torch.save(net.state_dict(), 
                 '{}/{}_{}_{:08d}_level{}.pth'.format(
                   args.model_dir, args.save_model_name, 
                   names[net_idx], index, level))


def get_prev_models(prev_levels=0):
  names=['encoder', 'binarizer', 'unet']
  en=[]
  bn=[]
  un=[]
  for level in range(prev_levels):
    prev_en, prev_bn, _, prev_un = get_models(
        args=args, v_compress=args.v_compress, 
        bits=args.bits,
        encoder_fuse_level=args.encoder_fuse_level,
        decoder_fuse_level=args.decoder_fuse_level,
        level = level)
    prev_en.load_state_dict(torch.load('{}/{}_{}_{:08d}_level{}.pth'.format(args.model_dir, load_model_name, names[0], index, level)))
    prev_bn.load_state_dict(torch.load('{}/{}_{}_{:08d}_level{}.pth'.format(args.model_dir, load_model_name, names[1], index, level)))
    prev_un.load_state_dict(torch.load('{}/{}_{}_{:08d}_level{}.pth'.format(args.model_dir, load_model_name, names[2], index, level)))

    prev_en = prev_en.cpu()
    prev_bn = prev_bn.cpu()
    prev_un = prev_un.cpu()

    prev_en.parameters().requires_grad = False
    prev_bn.parameters().requires_grad = False
    prev_un.parameters().requires_grad = False

    en.append(prev_en)
    bn.append(prev_bn)
    un.append(prev_un)

    return en, bn, un

en, bn, un = get_prev_models(args.prev_levels)

############### Training ###############

train_iter = 0
just_resumed = False
if args.load_model_name:
    print('Loading %s@iter %d' % (args.load_model_name,
                                  args.load_iter))

    resume(args.load_model_name, args.load_iter)
    train_iter = args.load_iter
    scheduler.last_epoch = train_iter - 1
    schedulerD.last_epoch = train_iter - 1
    just_resumed = True


is_dis_training = True
dis_times_per_gen = 3
dis_itr = 0
lamb = 0.1
while True:

    for batch, (crops, crops_gaussian, crops_laplacian, ctx_frames, 
      ctx_frames_gaussian, ctx_frames_laplacian, _) in enumerate(train_loader):
        #scheduler.step()
        #schedulerD.step()
        if is_dis_training == False:
          scheduler.step()                                                                                      
          schedulerD.step()
          train_iter += 1

        if train_iter > args.max_train_iters:
          break

        #batch_t0 = time.time()
        solver.zero_grad()
        solverD.zero_grad()

        ############################################################################
        # Init LSTM states.
        (encoder_h_1, encoder_h_2, encoder_h_3, encoder_h_4,
         decoder_h_1, decoder_h_2, decoder_h_3, decoder_h_4, decoder_h_5) = init_lstm(
            batch_size=(crops[0].size(0) * args.num_crops), height=crops[0].size(2),
            width=crops[0].size(3), args=args)

        old_encoder_hs = []
        for level in range(args.prev_levels):
          (en_h_1, en_h_2, en_h_3, en_h_4,_, _, _, _, _) = init_lstm(
            batch_size=(crops[0].size(0) * args.num_crops), height=crops[0].size(2),
            width=crops[0].size(3), args=args)
          old_encoder_hs.append([en_h_1, en_h_2, en_h_3, en_h_4])

        # Forward U-net.
        #if args.v_compress:
        #    unet_output1, unet_output2 = forward_ctx(unet, ctx_frames)
        #else:
        #unet_output1 = Variable(torch.zeros(args.batch_size,)).to(device)
        #unet_output2 = Variable(torch.zeros(args.batch_size,)).to(device)
        res, _, _, _, _ = prepare_inputs(crops, args, Variable(torch.zeros(args.batch_size,)).to(device), 
                                                          Variable(torch.zeros(args.batch_size,)).to(device))
        input_img = res

        if args.v_compress:
            unet_output1, unet_output2 = forward_ctx(unet, ctx_frames_laplacian[0])
        else:
            unet_output1 = Variable(torch.zeros(args.batch_size,)).to(device)
            unet_output2 = Variable(torch.zeros(args.batch_size,)).to(device)

        res, frame1, frame2, warped_unet_output1, warped_unet_output2 = prepare_inputs(
            crops_laplacian[0], args, unet_output1, unet_output2)


        old_res = []
        old_frames = []
        old_wr_un_out = []
        for level in range(args.prev_levels):
          if args.v_compress:
            old_unet_out1, old_unet_out2 = forward_ctx(un[level].cuda(), ([ctx_frames_gaussian]+ctx_frames_laplacian[1:][::-1])[level])
            un[level] = un[level].cpu()
          else:
            old_unet_out1 = Variable(torch.zeros(args.batch_size,)).cuda()
            old_unet_out2 = Variable(torch.zeros(args.batch_size,)).cuda()

          temp = prepare_inputs(([crops_gaussian]+crops_laplacian[1:][::-1]), args, unet_output1, unet_output2) ########### change prepare_inputs()
          old_res.append(temp[0])
          old_frames.append([temp[1], temp[2]])
          old_wr_un_out.append([temp[3], temp[4]])


        ############################################################################

        if is_dis_training == True: ####################### DISCRIMINATOR
          #print('dis..')
          #lossesD = []
          dis_itr += 1
          #bp_t0 = time.time()
          _, _, height, width = res.size()
          out_img = torch.zeros(1, 3, height, width).to(device) + 0.5

          for _ in range(args.iterations):
              if args.v_compress and args.stack:
                  encoder_input = torch.cat([frame1, res, frame2], dim=1)
              else:
                  encoder_input = res
              # Encode.
              encoded, encoder_h_1, encoder_h_2, encoder_h_3, encoder_h_4 = encoder(
                  encoder_input, encoder_h_1, encoder_h_2, encoder_h_3, encoder_h_4,
                  warped_unet_output1, warped_unet_output2) ############# warped_unet_output1?

              old_encoded=[]
              for level in range(args.prev_levels):
                  if args.v_compress and args.stack:
                      old_encoder_in = torch.cat([old_frames[level][0], old_res[level], old_frames[level][1]], dim=1)
                  else:
                      old_encoder_in = old_res[level]
                  # Encode.
                  old_encoder = en[level].cuda()
                  temp_en, en_h_1, en_h_2, en_h_3, en_h_4 = old_encoder(
                      old_encoder_in, old_encoder_hs[level][0], old_encoder_hs[level][1], old_encoder_hs[level][2], 
                      old_encoder_hs[level][3],old_wr_un_out[level][0], old_wr_un_out[level][1])
                  old_encoder_hs[level][0], old_encoder_hs[level][1], old_encoder_hs[level][2], old_encoder_hs[level][3] = en_h_1, en_h_2, en_h_3, en_h_4
                  old_encoded.append(temp_en)
                  en[level] = old_encoder.cpu()


              # Binarize.
              codes = binarizer(encoded)

              old_codes=[]
              for level in range(args.prev_levels):
                  old_binarizer = bn[level].cuda()
                  old_codes.append(old_binarizer(old_encoded[level]))
                  bn[level] = old_binarizer.cpu()

              new_codes = torch.cat(old_codes+[codes], dim=1)

              # Decode.
              (output, decoder_h_1, decoder_h_2, decoder_h_3, decoder_h_4, decoder_h_5) = decoder(
                  new_codes, decoder_h_1, decoder_h_2, decoder_h_3, decoder_h_4, decoder_h_5,
                  warped_unet_output1, warped_unet_output2)

              #adv_lossD_inst = (criterionGAN(netD(output.detach()), False) + criterionGAN(netD(res), True)) * 0.5
              output_gaussian_prev = output
              output_laps = []
              for j in range(args.prev_levels):
                  output_gaussian_new = cv2.GaussianBlur(output_gaussian_prev, (5,5), cv2.BORDER_DEFAULT)
                  output_laplacian = output_gaussian_prev - output_gaussian_new
                  output_gaussian_prev = output_gaussian_new
                  output_laps.append(np_to_torch(output_laplacian))
              output_gaussian = np_to_torch(output_new)
              
              res = res - output_laps[0]
              for level in range(args.prev_levels):
                  old_res[level] = old_res[level] - ([output_gaussian]+output_laps[1:][::-1])[level]
              #res = res - output
              out_img = out_img + output.data
              #lossesD.append(adv_lossD_inst)

          #bp_t1 = time.time()
          #adv_loss = sum(lossesD) / args.iterations
          adv_lossD = (criterionGAN(netD((out_img-0.5).detach()), False) + criterionGAN(netD(input_img), True)) * 0.5
          solverD.zero_grad()
          adv_lossD.backward()
          solverD.step()
          #batch_t1 = time.time()

          if dis_itr == dis_times_per_gen:
            is_dis_training = False
            dis_itr = 0

        else: ####################### GENERATOR
          #print('gen..')
          #losses = []
          losses_rec = []

          #bp_t0 = time.time()
          _, _, height, width = res.size()
          out_img = torch.zeros(1, 3, height, width).to(device) + 0.5
          '''
          for _ in range(args.iterations):
              if args.v_compress and args.stack:
                  encoder_input = torch.cat([frame1, res, frame2], dim=1)
              else:
                  encoder_input = res
              # Encode.
              encoded, encoder_h_1, encoder_h_2, encoder_h_3, encoder_h_4 = encoder(
                  encoder_input, encoder_h_1, encoder_h_2, encoder_h_3, encoder_h_4,
                  warped_unet_output1, warped_unet_output2)
              # Binarize.
              codes = binarizer(encoded)
              # Decode.
              (output, decoder_h_1, decoder_h_2, decoder_h_3, decoder_h_4, decoder_h_5) = decoder(
                  codes, decoder_h_1, decoder_h_2, decoder_h_3, decoder_h_4, decoder_h_5,
                  warped_unet_output1, warped_unet_output2)
          '''
          for _ in range(args.iterations):
              if args.v_compress and args.stack:
                  encoder_input = torch.cat([frame1, res, frame2], dim=1)
              else:
                  encoder_input = res
              # Encode.
              encoded, encoder_h_1, encoder_h_2, encoder_h_3, encoder_h_4 = encoder(
                  encoder_input, encoder_h_1, encoder_h_2, encoder_h_3, encoder_h_4,
                  warped_unet_output1, warped_unet_output2)

              old_encoded=[]
              for level in range(args.prev_levels):
                  if args.v_compress and args.stack:
                      old_encoder_in = torch.cat([old_frames[level][0], old_res[level], old_frames[level][1]], dim=1)
                  else:
                      old_encoder_in = old_res[level]
                  # Encode.
                  old_encoder = en[level].cuda()
                  temp_en, en_h_1, en_h_2, en_h_3, en_h_4 = old_encoder(
                      old_encoder_in, old_encoder_hs[level][0], old_encoder_hs[level][1], old_encoder_hs[level][2], 
                      old_encoder_hs[level][3],old_wr_un_out[level][0], old_wr_un_out[level][1])
                  old_encoder_hs[level][0], old_encoder_hs[level][1], old_encoder_hs[level][2], old_encoder_hs[level][3] = en_h_1, en_h_2, en_h_3, en_h_4
                  old_encoded.append(temp_en)
                  en[level] = old_encoder.cpu()


              # Binarize.
              codes = binarizer(encoded)

              old_codes=[]
              for level in range(args.prev_levels):
                  old_binarizer = bn[level].cuda()
                  old_codes.append(old_binarizer(old_encoded[level]))
                  bn[level] = old_binarizer.cpu()

              new_codes = torch.cat(old_codes+[codes], dim=1)

              # Decode.
              (output, decoder_h_1, decoder_h_2, decoder_h_3, decoder_h_4, decoder_h_5) = decoder(
                  new_codes, decoder_h_1, decoder_h_2, decoder_h_3, decoder_h_4, decoder_h_5,
                  warped_unet_output1, warped_unet_output2)

              #adv_lossD_inst = (criterionGAN(netD(output.detach()), False) + criterionGAN(netD(res), True)) * 0.5
              output_gaussian_prev = output
              output_laps = []
              for j in range(args.prev_levels):
                  output_gaussian_new = cv2.GaussianBlur(output_gaussian_prev, (5,5), cv2.BORDER_DEFAULT)
                  output_laplacian = output_gaussian_prev - output_gaussian_new
                  output_gaussian_prev = output_gaussian_new
                  output_laps.append(np_to_torch(output_laplacian))
              output_gaussian = np_to_torch(output_new)
              
              res = res - output_laps[0]
              for level in range(args.prev_levels):
                  old_res[level] = old_res[level] - ([output_gaussian]+output_laps[1:][::-1])[level]

              rec_loss_inst = sum([j.abs().mean() for j in [res]+old_res])
              #res = res - output
              #rec_loss_inst = res.abs().mean()
              out_img = out_img + output.data
              #losses.append(rec_loss_inst + lamb * adv_lossG_inst)
              losses_rec.append(rec_loss_inst)

          #bp_t1 = time.time()
          adv_lossG = criterionGAN(netD(out_img-0.5), True)
          #gen_loss = sum(losses) / args.iterations
          rec_loss = sum(losses_rec) / args.iterations
          #gen_loss = rec_loss
          gen_loss = rec_loss + lamb * adv_lossG

          p2p_networks.set_requires_grad(netD, False)
          solver.zero_grad()
          gen_loss.backward()
          for net in [encoder, binarizer, decoder, unet]:
              if net is not None:
                  torch.nn.utils.clip_grad_norm(net.parameters(), args.clip)
          solver.step()
          p2p_networks.set_requires_grad(netD, True)
          #batch_t1 = time.time()

          #Backprop: {:.4f} sec; Batch: {:.4f} sec'.
          print(
              '[TRAIN] Iter[{}]; LR: {}; Dis Loss: {:.6f}; Gen Loss: {:.6f}; Rec Loss: {:.6f};'. 
              format(train_iter, 
                     scheduler.get_lr()[0],
                     adv_lossD.item(), 
                     gen_loss.item(),
                     rec_loss.item()))

          if train_iter % 500 == 0:
              print('Loss at each step:')
              print(('{:.4f} ' * args.iterations +
                     '\n').format(* [l.item() for l in losses_rec])) #data[0]

          if train_iter % args.checkpoint_iters == 0:
              save(train_iter)

          if just_resumed or train_iter % args.eval_iters == 0 or train_iter == 20000:
              print('Start evaluation...')

              set_eval(nets)

              eval_loaders = get_eval_loaders()
              for eval_name, eval_loader in eval_loaders.items():
                  eval_begin = time.time()
                  eval_loss, mssim, psnr = run_eval(nets, (en, bn, un), eval_loader, args,
                      output_suffix='iter%d' % train_iter)

                  print('Evaluation @iter %d done in %d secs' % (
                      train_iter, time.time() - eval_begin))
                  print('%s Loss   : ' % eval_name
                        + '\t'.join(['%.5f' % el for el in eval_loss.tolist()]))
                  print('%s MS-SSIM: ' % eval_name
                        + '\t'.join(['%.5f' % el for el in mssim.tolist()]))
                  print('%s PSNR   : ' % eval_name
                        + '\t'.join(['%.5f' % el for el in psnr.tolist()]))

              set_train(nets)
              just_resumed = False

          is_dis_training = True

    if train_iter > args.max_train_iters:
      print('Training done.')
      break


