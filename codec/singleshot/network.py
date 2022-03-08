import torch
import torch.nn as nn
import torch.nn.functional as F

from modules import ConvLSTMCell, Sign


class EncoderCell(nn.Module):
    def __init__(self, v_compress, stack, fuse_encoder, fuse_level):
        super(EncoderCell, self).__init__()

        # Init.
        self.v_compress = v_compress
        self.fuse_encoder = fuse_encoder
        self.fuse_level = fuse_level
        if fuse_encoder:
            print('\tEncoder fuse level: {}'.format(self.fuse_level))

        # Layers.
        self.conv = nn.Conv2d(
            9 if stack else 3, 
            64, 
            kernel_size=3, stride=2, padding=1, bias=False)

        self.rnn1 = ConvLSTMCell(
            128 if fuse_encoder and v_compress else 64,
            256,
            kernel_size=3,
            stride=2,
            padding=1,
            hidden_kernel_size=1,
            bias=False)

        self.rnn2 = ConvLSTMCell(
            ((384 if fuse_encoder and v_compress else 256) 
             if self.fuse_level >= 2 else 256),
            512,
            kernel_size=3,
            stride=2,
            padding=1,
            hidden_kernel_size=1,
            bias=False)

        self.rnn3 = ConvLSTMCell(
            ((768 if fuse_encoder and v_compress else 512) 
             if self.fuse_level >= 3 else 512),
            512,
            kernel_size=3,
            stride=2,
            padding=1,
            hidden_kernel_size=1,
            bias=False)


    def forward(self, input, hidden1, hidden2, hidden3,
                unet_output1, unet_output2):

        x = self.conv(input)
        # Fuse
        if self.v_compress and self.fuse_encoder:
            x = torch.cat([x, unet_output1[2], unet_output2[2]], dim=1)

        hidden1 = self.rnn1(x, hidden1)
        x = hidden1[0]
        # Fuse.
        if self.v_compress and self.fuse_encoder and self.fuse_level >= 2:
            x = torch.cat([x, unet_output1[1], unet_output2[1]], dim=1)

        hidden2 = self.rnn2(x, hidden2)
        x = hidden2[0]
        # Fuse.
        if self.v_compress and self.fuse_encoder and self.fuse_level >= 3:
            x = torch.cat([x, unet_output1[0], unet_output2[0]], dim=1)

        hidden3 = self.rnn3(x, hidden3)
        x = hidden3[0]
        return x, hidden1, hidden2, hidden3


class Binarizer(nn.Module):
    def __init__(self, bits):
        super(Binarizer, self).__init__()
        self.conv = nn.Conv2d(512, bits, kernel_size=1, bias=False)
        self.sign = Sign()

    def forward(self, input):
        feat = self.conv(input)
        x = F.tanh(feat)
        return self.sign(x)


class DecoderCell(nn.Module):
    def __init__(self, v_compress, shrink, bits, fuse_level):

        super(DecoderCell, self).__init__()

        # Init.
        self.v_compress = v_compress
        self.fuse_level = fuse_level
        print('\tDecoder fuse level: {}'.format(self.fuse_level))

        # Layers.
        self.conv1 = nn.Conv2d(
            bits, 512, kernel_size=1, stride=1, padding=0, bias=False)

        self.rnn1 = ConvLSTMCell(
            512,
            512,
            kernel_size=3,
            stride=1,
            padding=1,
            hidden_kernel_size=1,
            bias=False)

        self.rnn2 = ConvLSTMCell(
            (((128 + 256 // shrink * 2) if v_compress else 128) 
             if self.fuse_level >= 3 else 128), #out1=256
            512,
            kernel_size=3,
            stride=1,
            padding=1,
            hidden_kernel_size=1,
            bias=False)

        self.rnn3 = ConvLSTMCell(
            (((128 + 128//shrink*2) if v_compress else 128) 
             if self.fuse_level >= 2 else 128), #out2=128
            256,
            kernel_size=3,
            stride=1,
            padding=1,
            hidden_kernel_size=3,
            bias=False)

        self.rnn4 = ConvLSTMCell(
            (64 + 64//shrink*2) if v_compress else 64, #out3=64
            128,
            kernel_size=3,
            stride=1,
            padding=1,
            hidden_kernel_size=3,
            bias=False)

        self.conv2 = nn.Conv2d(
            32,
            3, 
            kernel_size=1, stride=1, padding=0, bias=False)

    def forward(self, input, hidden1, hidden2, hidden3, hidden4,
                unet_output1, unet_output2):

        x = self.conv1(input)
        hidden1 = self.rnn1(x, hidden1)

        # rnn 2
        x = hidden1[0]
        x = F.pixel_shuffle(x, 2)

        if self.v_compress and self.fuse_level >= 3:
            x = torch.cat([x, unet_output1[0], unet_output2[0]], dim=1)

        hidden2 = self.rnn2(x, hidden2)

        # rnn 3
        x = hidden2[0]
        x = F.pixel_shuffle(x, 2)

        if self.v_compress and self.fuse_level >= 2:
            x = torch.cat([x, unet_output1[1], unet_output2[1]], dim=1)

        hidden3 = self.rnn3(x, hidden3)

        # rnn 4
        x = hidden3[0]
        x = F.pixel_shuffle(x, 2)

        if self.v_compress:
            x = torch.cat([x, unet_output1[2], unet_output2[2]], dim=1)

        hidden4 = self.rnn4(x, hidden4)

        # final
        x = hidden4[0]
        x = F.pixel_shuffle(x, 2)

        x = F.tanh(self.conv2(x)) / 2
        return x, hidden1, hidden2, hidden3, hidden4




#variable group ---- use this one for now --- best performing


class DecoderCell2(nn.Module):
    def __init__(self, v_compress, shrink, bits, fuse_level):

        super(DecoderCell2, self).__init__()

        # Init.
        self.v_compress = v_compress
        self.fuse_level = fuse_level
        print('\tDecoder fuse level: {}'.format(self.fuse_level))

        # Layers.
        #self.conv1 = nn.Conv2d(
        #    bits*10, 512, kernel_size=1, stride=1, padding=0, bias=False)

        self.conv1 = nn.Conv2d(bits*10, 64*10, groups=10, kernel_size=1, stride=1, padding=0, bias=False) # groups=10
        self.conv2 = nn.Conv2d(64*10, 128*10, groups=10, kernel_size=1, stride=1, padding=0, bias=False)
        self.conv3 = nn.Conv2d(128*10, 512*10, groups=10, kernel_size=1, stride=1, padding=0, bias=False)
        #self.bn1 = nn.BatchNorm2d(64*10)
        #self.bn2 = nn.BatchNorm2d(128*10)
        #self.bn3 = nn.BatchNorm2d(512*10)

        self.rnn1 = ConvLSTMCell(
            512,
            512,
            kernel_size=3,
            stride=1,
            padding=1,
            hidden_kernel_size=1,
            bias=False)

        self.rnn2 = ConvLSTMCell(
            (((128 + 256 // shrink * 2) if v_compress else 128) 
             if self.fuse_level >= 3 else 128), #out1=256
            512,
            kernel_size=3,
            stride=1,
            padding=1,
            hidden_kernel_size=1,
            bias=False)

        self.rnn3 = ConvLSTMCell(
            (((128 + 128//shrink*2) if v_compress else 128) 
             if self.fuse_level >= 2 else 128), #out2=128
            256,
            kernel_size=3,
            stride=1,
            padding=1,
            hidden_kernel_size=3,
            bias=False)

        self.rnn4 = ConvLSTMCell(
            (64 + 64//shrink*2) if v_compress else 64, #out3=64
            128,
            kernel_size=3,
            stride=1,
            padding=1,
            hidden_kernel_size=3,
            bias=False)

        self.conv_end = nn.Conv2d(
            32,
            3, 
            kernel_size=1, stride=1, padding=0, bias=False)

    def forward(self, input, hidden1, hidden2, hidden3, hidden4,
                unet_output1, unet_output2):

        b,d,h,w = input.shape
        x = F.relu(self.conv1(input)) #F.tanh
        x = F.relu(self.conv2(x))
        x = F.relu(self.conv3(x))
        x = x.reshape(b,10,-1,h,w).sum(1)
        hidden1 = self.rnn1(x, hidden1)

        # rnn 2
        x = hidden1[0]
        x = F.pixel_shuffle(x, 2)

        if self.v_compress and self.fuse_level >= 3:
            x = torch.cat([x, unet_output1[0], unet_output2[0]], dim=1)

        hidden2 = self.rnn2(x, hidden2)

        # rnn 3
        x = hidden2[0]
        x = F.pixel_shuffle(x, 2)

        if self.v_compress and self.fuse_level >= 2:
            x = torch.cat([x, unet_output1[1], unet_output2[1]], dim=1)

        hidden3 = self.rnn3(x, hidden3)

        # rnn 4
        x = hidden3[0]
        x = F.pixel_shuffle(x, 2)

        if self.v_compress:
            x = torch.cat([x, unet_output1[2], unet_output2[2]], dim=1)

        hidden4 = self.rnn4(x, hidden4)

        # final
        x = hidden4[0]
        x = F.pixel_shuffle(x, 2)

        x = F.tanh(self.conv_end(x)) / 2
        return x, hidden1, hidden2, hidden3, hidden4





#conv or res based
'''
class DecoderCell2(nn.Module):
    def __init__(self, v_compress, shrink, bits, fuse_level, groups=10):

        super(DecoderCell2, self).__init__()

        # Init.
        self.v_compress = v_compress
        self.fuse_level = fuse_level
        self.groups = groups
        print('\tDecoder fuse level: {}'.format(self.fuse_level))

        # Layers.
        self.conv_st_1 = nn.Conv2d(bits*10, 64*groups, kernel_size=1, stride=1, padding=0, bias=False, groups=groups)
        self.conv_st_2 = nn.Conv2d(64*groups, 128*groups, kernel_size=1, stride=1, padding=0, bias=False, groups=groups)
        self.conv_st_3 = nn.Conv2d(128*groups, 512*groups, kernel_size=1, stride=1, padding=0, bias=False, groups=groups)
        #self.conv_st_4 = nn.Conv2d(512*groups, 512*groups, kernel_size=1, stride=1, padding=0, bias=False, groups=groups)

        #self.conv1 = nn.Conv2d(512, 512, kernel_size=3, stride=1, padding=1, bias=False)
        self.conv1 = nn.Conv2d(512*groups, 512*groups, groups=groups, kernel_size=3, stride=1, padding=1, bias=False)

        self.conv2_g1 = nn.Conv2d(128*groups, 512*groups, groups=groups, kernel_size=3, stride=1, padding=1, bias=False)
        self.conv2_g2 = nn.Conv2d(512*groups, 128*groups, groups=groups, kernel_size=3, stride=1, padding=1, bias=False)

        self.conv2 = nn.Conv2d((((128 + 256 // shrink * 2) if v_compress else 128) 
                        if self.fuse_level >= 3 else 128), 
                        512, kernel_size=3, stride=1, padding=1, bias=False) ##### 128->512
        self.conv2_2 = nn.Conv2d(512, 512, kernel_size=3, stride=1, padding=1, bias=False)


        self.conv3 = nn.Conv2d((((128 + 128//shrink*2) if v_compress else 128) 
                        if self.fuse_level >= 2 else 128), 
                        256, kernel_size=3, stride=1, padding=1, bias=False)
        self.conv3_2 = nn.Conv2d(256, 256, kernel_size=3, stride=1, padding=1, bias=False)


        self.conv4 = nn.Conv2d((64 + 64//shrink*2) if v_compress else 64, 
                        128, kernel_size=3, stride=1, padding=1, bias=False)

        #self.res1 = ResNetBlock(512*groups, 512*groups, stride=1, groups=groups)
        #self.res2 = ResNetBlock((((128 + 256 // shrink * 2) if v_compress else 128) if self.fuse_level >= 3 else 128), 512, stride=1)
        #self.res3 = ResNetBlock((((128 + 128//shrink*2) if v_compress else 128) if self.fuse_level >= 2 else 128), 256, stride=1)
        #self.res4 = ResNetBlock((64 + 64//shrink*2) if v_compress else 64, 128, stride=1) 
        self.conv_end = nn.Conv2d(32,
                        3, kernel_size=1, stride=1, padding=0, bias=False)


    def forward(self, input, hidden1, hidden2, hidden3, hidden4, unet_output1, unet_output2):

        b,d,h,w = input.shape
        x = F.tanh(self.conv_st_1(input))
        x = F.tanh(self.conv_st_2(x))
        x = F.tanh(self.conv_st_3(x))   
        #x = F.tanh(self.conv_st_4(x)) ####
        #x = self.res1(x)
        #x = F.tanh(self.bn1(self.conv1(x)))
        #if self.groups>1:
        #    x = x.reshape(b,10,-1,h,w).sum(1)
        x = F.tanh(self.conv1(x))
        #print(x.shape)

        x = F.pixel_shuffle(x, 2)
        #print(x.shape)

        x = F.tanh(self.conv2_g1(x))
        x = F.tanh(self.conv2_g2(x))
        #print(x.shape)
        if self.groups>1:
            x = x.reshape(b,10,-1,h*2,w*6).sum(1)
        #print(x.shape)

        if self.v_compress and self.fuse_level >= 3:
            x = torch.cat([x, unet_output1[0], unet_output2[0]], dim=1)

        #x = self.res2(x)
        #print(x.shape)
        x = F.tanh(self.conv2(x))
        x = F.tanh(self.conv2_2(x)) ####
        x = F.pixel_shuffle(x, 2)

        if self.v_compress and self.fuse_level >= 2:
            x = torch.cat([x, unet_output1[1], unet_output2[1]], dim=1)

        #x = self.res3(x)
        x = F.tanh(self.conv3(x))
        x = F.tanh(self.conv3_2(x)) ####
        x = F.pixel_shuffle(x, 2)

        if self.v_compress:
            x = torch.cat([x, unet_output1[2], unet_output2[2]], dim=1)

        #x = self.res4(x)
        x = F.tanh(self.conv4(x))
        x = F.pixel_shuffle(x, 2)

        x = F.tanh(self.conv_end(x)) / 2
        return x, hidden1, hidden2, hidden3, hidden4


class ResNetBlock(nn.Module):
    def __init__(self, fin, fout, stride=1, fhidden=None, bn=True, groups=1): ####### stride
        super().__init__()
        # Attributes
        self.bn = bn
        self.is_bias = not bn
        self.learned_shortcut = (fin != fout)
        self.fin = fin
        self.fout = fout
        self.groups = groups
        if fhidden is None:
            self.fhidden = min(fin, fout)
        else:
            self.fhidden = fhidden

        # Submodules
        self.conv_0 = nn.Conv2d(self.fin, self.fhidden, 3, stride=1, padding=1, bias=self.is_bias, groups=groups)
        if self.bn:
            self.bn2d_0 = nn.BatchNorm2d(self.fhidden)
        self.conv_1 = nn.Conv2d(self.fhidden, self.fout, 3, stride=stride, padding=1, bias=self.is_bias, groups=groups)
        if self.bn:
            self.bn2d_1 = nn.BatchNorm2d(self.fout)
        if self.learned_shortcut:
            self.conv_s = nn.Conv2d(self.fin, self.fout, 1, stride=stride, padding=0, bias=False, groups=groups)
            if self.bn:
                self.bn2d_s = nn.BatchNorm2d(self.fout)
        self.relu = nn.LeakyReLU(0.2, inplace=True)
        #self.tanh = nn.Tanh()
        
    def forward(self, x):
        x_s = self._shortcut(x)
        dx = self.conv_0(x)
        if self.bn:
            dx = self.bn2d_0(dx)
        dx = self.relu(dx)
        dx = self.conv_1(dx)
        if self.bn:
            dx = self.bn2d_1(dx)
        out = self.relu(x_s + dx)
        return out

    def _shortcut(self, x):
        if self.learned_shortcut:
            x_s = self.conv_s(x)
            if self.bn:
                x_s = self.bn2d_s(x_s)
        else:
            x_s = x
        return x_s
'''





'''
class DecoderCell2(nn.Module):
    def __init__(self, v_compress, shrink, bits, fuse_level):

        super(DecoderCell2, self).__init__()

        # Init.
        self.v_compress = v_compress
        self.fuse_level = fuse_level
        print('\tDecoder fuse level: {}'.format(self.fuse_level))

        # Layers.
        self.conv_st = nn.Conv2d(bits*10, 
                        512, kernel_size=1, stride=1, padding=0, bias=False)
        self.bn_st = nn.BatchNorm2d(512)

        #self.conv1 = nn.Conv2d(512, 
        #                512, kernel_size=3, stride=1, padding=1, bias=False)
        #self.bn1 = nn.BatchNorm2d(512)
        self.res1 = ResNetBlock(512, 512, stride=1)

        #self.conv1_2 = nn.Conv2d(512, 
        #                512, kernel_size=3, stride=1, padding=1, bias=False)
        #self.bn1_2 = nn.BatchNorm2d(512)

        #self.conv2 = nn.Conv2d((((512 + 256 // shrink * 2) if v_compress else 512) 
        #                if self.fuse_level >= 3 else 512), 
        #                512, kernel_size=3, stride=1, padding=1, bias=False) ##### 128->512
        #self.bn2 = nn.BatchNorm2d(512)
        self.res2 = ResNetBlock((((128 + 256 // shrink * 2) if v_compress else 128) 
                        if self.fuse_level >= 3 else 128), 512, stride=1)

        #self.conv2_2 = nn.Conv2d(512, 
        #                512, kernel_size=3, stride=1, padding=1, bias=False)
        #self.bn2_2 = nn.BatchNorm2d(512)

        #self.conv3 = nn.Conv2d((((128 + 128//shrink*2) if v_compress else 128) 
        #                if self.fuse_level >= 2 else 128), 
        #                256, kernel_size=3, stride=1, padding=1, bias=False)
        #self.bn3 = nn.BatchNorm2d(256)
        self.res3 = ResNetBlock((((128 + 128//shrink*2) if v_compress else 128) 
                        if self.fuse_level >= 2 else 128), 256, stride=1)

        #self.conv4 = nn.Conv2d((64 + 64//shrink*2) if v_compress else 64, 
        #                128, kernel_size=3, stride=1, padding=1, bias=False)
        #self.bn4 = nn.BatchNorm2d(128)
        self.res4 = ResNetBlock((64 + 64//shrink*2) if v_compress else 64, 128, stride=1)

        self.conv_end = nn.Conv2d(32,
                        3, kernel_size=1, stride=1, padding=0, bias=False)


    def forward(self, input, hidden1, hidden2, hidden3, hidden4, unet_output1, unet_output2):
        #print(input.shape)
        x = F.relu(self.bn_st(self.conv_st(input)), inplace=True)
        #print(x.shape)
        #x = F.relu(self.bn1(self.conv1(x)), inplace=True)
        x = self.res1(x)
        #print(x.shape)
        #x = F.relu(self.bn1_2(self.conv1_2(x)), inplace=True)
        x = F.pixel_shuffle(x, 2)
        #print(x.shape)

        if self.v_compress and self.fuse_level >= 3:
            x = torch.cat([x, unet_output1[0], unet_output2[0]], dim=1)
        #print(x.shape)
        #x = F.relu(self.bn2(self.conv2(x)), inplace=True)
        x = self.res2(x)
        #x = F.relu(self.bn2_2(self.conv2_2(x)), inplace=True)
        x = F.pixel_shuffle(x, 2)

        if self.v_compress and self.fuse_level >= 2:
            x = torch.cat([x, unet_output1[1], unet_output2[1]], dim=1)

        #x = F.relu(self.bn3(self.conv3(x)), inplace=True)
        x = self.res3(x)
        x = F.pixel_shuffle(x, 2)

        if self.v_compress:
            x = torch.cat([x, unet_output1[2], unet_output2[2]], dim=1)

        #x = F.relu(self.bn4(self.conv4(x)), inplace=True)
        x = self.res4(x)
        x = F.pixel_shuffle(x, 2)

        x = F.tanh(self.conv_end(x)) / 2
        return x, hidden1, hidden2, hidden3, hidden4


class ResNetBlock(nn.Module):
    def __init__(self, fin, fout, stride=1, fhidden=None, bn=True): ####### stride
        super().__init__()
        # Attributes
        self.bn = bn
        self.is_bias = not bn
        self.learned_shortcut = (fin != fout)
        self.fin = fin
        self.fout = fout
        if fhidden is None:
            self.fhidden = min(fin, fout)
        else:
            self.fhidden = fhidden

        # Submodules
        self.conv_0 = nn.Conv2d(self.fin, self.fhidden, 3, stride=1, padding=1, bias=self.is_bias)
        if self.bn:
            self.bn2d_0 = nn.BatchNorm2d(self.fhidden)
        self.conv_1 = nn.Conv2d(self.fhidden, self.fout, 3, stride=stride, padding=1, bias=self.is_bias)
        if self.bn:
            self.bn2d_1 = nn.BatchNorm2d(self.fout)
        if self.learned_shortcut:
            self.conv_s = nn.Conv2d(self.fin, self.fout, 1, stride=stride, padding=0, bias=False)
            if self.bn:
                self.bn2d_s = nn.BatchNorm2d(self.fout)
        self.relu = nn.LeakyReLU(0.2, inplace=True)
        
    def forward(self, x):
        x_s = self._shortcut(x)
        dx = self.conv_0(x)
        if self.bn:
            dx = self.bn2d_0(dx)
        dx = self.relu(dx)
        dx = self.conv_1(dx)
        if self.bn:
            dx = self.bn2d_1(dx)
        out = self.relu(x_s + dx)
        return out

    def _shortcut(self, x):
        if self.learned_shortcut:
            x_s = self.conv_s(x)
            if self.bn:
                x_s = self.bn2d_s(x_s)
        else:
            x_s = x
        return x_s

'''
