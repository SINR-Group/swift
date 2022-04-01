import os
import logging
import numpy as np
import multiprocessing as mp
os.environ['CUDA_VISIBLE_DEVICES']=''
import tensorflow as tf
import tensorflow.compat.v1 as tf
tf.disable_v2_behavior()
import simulator as env
import net
import common
from config import *


def testing(epoch, nn_model, log_file):
    # clean up the test results folder
    os.system('rm -r ' + TEST_LOG_FOLDER)
    os.system('mkdir ' + TEST_LOG_FOLDER)
    # run test script
    os.system('%s test_grad.py %s %s 0' % (PYTHON_EXECUTABLE_PATH, nn_model, DEV_TRACES))

    # append test performance to the log
    rewards = []
    test_log_files = os.listdir(TEST_LOG_FOLDER)
    for test_log_file in test_log_files:
        if os.path.isdir(TEST_LOG_FOLDER + test_log_file): continue
        reward = []
        with open(TEST_LOG_FOLDER + test_log_file, 'rb') as f:
            for line in f:
                parse = line.split()
                if 'fill' in str(line) or len(parse) < 7:
                    continue
                try:
                    reward.append(float(parse[6]))
                except IndexError:
                    break
        rewards.append(np.sum(reward[1:]))

    rewards = np.array(rewards)
    rewards_min = np.min(rewards)
    rewards_5per = np.percentile(rewards, 5)
    rewards_mean = np.mean(rewards)
    rewards_median = np.percentile(rewards, 50)
    rewards_95per = np.percentile(rewards, 95)
    rewards_max = np.max(rewards)

    log_file.write(str(epoch) + '\t' +
                   str(rewards_min) + '\t' +
                   str(rewards_5per) + '\t' +
                   str(rewards_mean) + '\t' +
                   str(rewards_median) + '\t' +
                   str(rewards_95per) + '\t' +
                   str(rewards_max) + '\n')
    log_file.flush()
    return rewards_mean

def central_agent(net_params_queues, exp_queues):

    assert len(net_params_queues) == NUM_AGENTS
    assert len(exp_queues) == NUM_AGENTS
    largest_avg_reward=0
    logging.basicConfig(filename=LOG_FILE + '_central',
                        filemode='w',
                        level=logging.INFO)

    with tf.Session() as sess, open(LOG_FILE + '_test', 'w') as test_log_file:

        actor = net.ActorNetwork(sess,
                                 state_dim=[FEATURE_DIM, REGRET_WINDOW_SIZE], action_dim=OUTPUT_DIM,
                                 learning_rate=ACTOR_LR_RATE)
        critic = net.CriticNetwork(sess,
                                   state_dim=[FEATURE_DIM, REGRET_WINDOW_SIZE],
                                   learning_rate=CRITIC_LR_RATE)


        sess.run(tf.global_variables_initializer())

        print("2---------------------------------------")
        saver = tf.train.Saver()  # save neural net parameters
        print("3---------------------------------------")
        # restore neural net parameters
        # nn_model = NN_MODEL
        nn_model = None
        if nn_model is not None:  # nn_model is the path to file
            saver.restore(sess, nn_model)
            print("Model restored.")

        epoch = 0

        # assemble experiences from agents, compute the gradients
        while True:
            # synchronize the network parameters of work agent
            actor_net_params = actor.get_network_params()
            critic_net_params = critic.get_network_params()

            for i in range(NUM_AGENTS):
                net_params_queues[i].put([actor_net_params, critic_net_params])

            # record average reward and td loss change
            # in the experiences from the agents
            total_batch_len = 0.0
            total_reward = 0.0
            total_td_loss = 0.0
            total_entropy = 0.0
            total_agents = 0.0

            # assemble experiences from the agents
            actor_gradient_batch = []
            critic_gradient_batch = []

            for i in range(NUM_AGENTS):
                #print("waiting to get...")
                s_batch, a_batch, r_batch,r_batch_lable, r_for_train_batch, terminal, info,last_fills_count = exp_queues[i].get()
                #print("got it!")

                actor_gradient, critic_gradient, td_batch = \
                    net.compute_gradients(
                        s_batch=np.stack(s_batch, axis=0),
                        a_batch=np.vstack(a_batch),
                        r_batch=np.vstack(r_for_train_batch),
                        r_batch_lable=np.vstack(r_batch_lable),
                        terminal=terminal, actor=actor, critic=critic,last_fills=last_fills_count)

                actor_gradient_batch.append(actor_gradient)
                critic_gradient_batch.append(critic_gradient)

                total_reward += np.sum(r_batch)
                total_td_loss += np.sum(td_batch)
                total_batch_len += len(r_batch)
                total_agents += 1.0
                total_entropy += np.sum(info['entropy'])

            # compute aggregated gradient
            assert NUM_AGENTS == len(actor_gradient_batch)
            assert len(actor_gradient_batch) == len(critic_gradient_batch)


            for i in range(len(actor_gradient_batch)):
                actor.apply_gradients(actor_gradient_batch[i])
                critic.apply_gradients(critic_gradient_batch[i])

            # log training information
            epoch += 1
            if epoch % 100 ==0:
                actor.decay_entropy()
            if epoch >= ENTROPY_DECAY_ITERATIONS: exit()
            #if epoch % 20000 ==0:
            #    exit()
            avg_reward = total_reward  / total_agents
            avg_td_loss = total_td_loss / total_batch_len
            avg_entropy = total_entropy / total_batch_len

            logging.info('Epoch: ' + str(epoch) +
                         ' TD_loss: ' + str(avg_td_loss) +
                         ' Avg_reward: ' + str(avg_reward) +
                         ' Avg_entropy: ' + str(avg_entropy))


            if epoch % MODEL_SAVE_INTERVAL == 0:
                # Save the neural net parameters to disk.
                print("\n\n\n-----------saving model ----------------------")
                os.system("rm -r ./results/grad*")
                save_path = saver.save(sess, RESULTS_DIR + "/grad_" +
                                       str(epoch) + ".ckpt")
                print("----------------------------------------------------\n\n")
                logging.info("Model saved in file: " + save_path)

                print("\n\n\n-----------------testing model----------------")
                # test_reward=testing(epoch,
                #     RESULTS_DIR + "grad_" + str(epoch) + ".ckpt",
                #     test_log_file)
                # if test_reward>largest_avg_reward and epoch > 30000:
                #     largest_avg_reward=test_reward
                #     os.system("rm -r ./best_model/*")
                #     os.system("cp ./results/"+"grad_"+str(epoch)+"* ./best_model/")
                #     os.system("touch ./results/"+str(test_reward))
                print("---------------------------------------------------\n\n")


def agent(agent_id, all_cooked_time, all_cooked_bw, all_file_names,net_params_queue, exp_queue):

    net_env = env.Environment(all_cooked_time=all_cooked_time,
                              all_cooked_bw=all_cooked_bw,
                              all_file_names = all_file_names,
                              log_folder = RESULTS_DIR,
                              mode = TRAIN,
                              random_seed=agent_id)

    with tf.Session() as sess, open(LOG_FILE + '_agent_' + str(agent_id), 'w') as log_file:
        actor = net.ActorNetwork(sess,
                                 state_dim=[FEATURE_DIM, REGRET_WINDOW_SIZE], action_dim=OUTPUT_DIM,
                                 learning_rate=ACTOR_LR_RATE)
        critic = net.CriticNetwork(sess,
                                   state_dim=[FEATURE_DIM, REGRET_WINDOW_SIZE],
                                   learning_rate=CRITIC_LR_RATE)

        # initial synchronization of the network parameters from the coordinator
        actor_net_params, critic_net_params = net_params_queue.get()
        actor.set_network_params(actor_net_params)
        critic.set_network_params(critic_net_params)

        last_bit_rate = DEFAULT_QUALITY
        bit_rate = DEFAULT_QUALITY

        action_next_or_fill = 0
        action_vec = np.zeros(OUTPUT_DIM)
        action = DEFAULT_QUALITY
        action_vec[action] = 1

        s_batch = [np.zeros((FEATURE_DIM, REGRET_WINDOW_SIZE))]
        a_batch = [action_vec]
        r_batch = []
        r_batch_lable= []
        r_for_train_batch=[]
        entropy_record = []

        time_stamp = 0
        last_fills_count = 0
        while True:  # experience video streaming forever

            delay, sleep_time, buffer_size, rebuf, \
            video_chunk_size, next_video_chunk_sizes, \
            download_complete,play_complete, video_chunk_remain,regret_chunk_remain_time,regret_chunk_bitrate,regret_chunk_alternate_size, \
            video_sent_size,\
            regret_succeed,old_bit,last_bit ,next_bit,latest_chunk_updated,current_buffer_num,regret_chunk_scalable= \
                net_env.get_video_chunk(bit_rate,action_next_or_fill)    # action_next_or_regret
            '''
            print("\n\nregret_succeed:",regret_succeed)
            print("delay:",delay)
            print("bitrate:",bit_rate)
            print("old_bit:",old_bit)
            print("last_bitrate:",last_bit)
            print("next_bitrate:",next_bit)
            print("last_bit_rate:",last_bit_rate)
            print("rebuf:",rebuf)
            print("current_buffer_num",current_buffer_num)
            print("latest_chunk_updated:",latest_chunk_updated)
            print("action_next_or_regret:",action_next_or_fill)
            print("video_chunk_remain:",video_chunk_remain)
            print("video_sent_size:",video_sent_size)
            print("video_chunk_size:",video_chunk_size)

           #print("regret_chunk_alternate_size---------------------------------:")
           #for i in range(REGRET_WINDOW_SIZE):
           #    print(str(i+1)+":",regret_chunk_alternate_size[i])
           #print("------------------------------------------------------------")

            print("regret_chunk_remain_time:",regret_chunk_remain_time)
            print("regret_chunk_bitrate:",regret_chunk_bitrate)
            print("download_complete:",download_complete)
            print("play complete:",play_complete)
            print("regret scalable:",regret_chunk_scalable)

            '''


            '''
            else:
                delay, sleep_time, buffer_size, rebuf, \
                video_chunk_size, next_video_chunk_sizes, \
                download_complete, play_complete, video_chunk_remain, regret_chunk_remain_time, regret_chunk_bitrate, regret_chunk_alternate_size, \
                video_sent_size, \
                regret_succeed, old_bit, last_bit, next_bit, latest_chunk_updated, current_buffer_num = \
                    take_next_action(regret_chunk_bitrate,current_buffer_num,net_env,bit_rate)  # action_next_or_regret)
           '''


            time_stamp += delay  # in ms
            time_stamp += sleep_time  # in ms



            if regret_succeed != 0: #its quality upgrade action
                reward = 0
                reward_for_train = 0
                if regret_succeed == 1:
                    assert (next_bit == None and latest_chunk_updated == 1) or next_bit != None
                    # -- reward --
                    if latest_chunk_updated:
                        last_bit_rate = bit_rate

                    if old_bit != bit_rate:
                        log_bit_rate = np.log2(VIDEO_BIT_RATE[bit_rate] / float(VIDEO_BIT_RATE[0]))
                        log_old_bit = np.log2(VIDEO_BIT_RATE[old_bit] / float(VIDEO_BIT_RATE[0]))
                        if last_bit != None:
                            log_last_bit = np.log2(VIDEO_BIT_RATE[last_bit] / float(VIDEO_BIT_RATE[0]))

                            reward += (- SMOOTH_PENALTY * np.abs(
                                log_bit_rate - log_last_bit)*(float(max(VIDEO_BIT_RATE[bit_rate],VIDEO_BIT_RATE[last_bit]))/min(VIDEO_BIT_RATE[bit_rate],VIDEO_BIT_RATE[last_bit]))  + SMOOTH_PENALTY * np.abs(
                                log_old_bit - log_last_bit)*(float(max(VIDEO_BIT_RATE[old_bit],VIDEO_BIT_RATE[last_bit]))/min(VIDEO_BIT_RATE[old_bit],VIDEO_BIT_RATE[last_bit])))

                        if next_bit != None:
                            log_next_bit = np.log2(VIDEO_BIT_RATE[next_bit] / float(VIDEO_BIT_RATE[0]))
                            reward += (- SMOOTH_PENALTY * np.abs(
                                log_next_bit - log_bit_rate)*(float(max(VIDEO_BIT_RATE[bit_rate],VIDEO_BIT_RATE[next_bit]))/min(VIDEO_BIT_RATE[bit_rate],VIDEO_BIT_RATE[next_bit])) + SMOOTH_PENALTY * np.abs(
                                log_next_bit - log_old_bit)*(float(max(VIDEO_BIT_RATE[old_bit],VIDEO_BIT_RATE[next_bit]))/min(VIDEO_BIT_RATE[old_bit],VIDEO_BIT_RATE[next_bit])))
                        reward += (log_bit_rate - log_old_bit)


                reward_for_train = reward

                if regret_succeed == 2:
                    assert reward == 0
                    reward_for_train -= 0  # You can add extra penalty to help avoid exceeding upgrade deadline
                    log_file.write("fill fail!\n")
                else:
                    log_file.write("fill:" + str(VIDEO_BIT_RATE[old_bit]) + " -> " + str(
                        VIDEO_BIT_RATE[bit_rate]) + '\t' + "actual reward:" + str(reward) + '\n')

                r_batch.append(reward)
                r_batch_lable.append(1)  # 1 means the reward is generated by upgrade action
                r_for_train_batch.append(reward_for_train)
                # retrieve previous state
                if len(s_batch) == 0:
                    state = [np.zeros((FEATURE_DIM, REGRET_WINDOW_SIZE))]
                else:
                    state = np.array(s_batch[-1], copy=True)

                if delay != 0:
                    # dequeue history record
                    state = np.roll(state, -1, axis=1)

                    # this should be FEATURE_DIM number of terms
                    state[0, -1] = state[0, -2]  # VIDEO_BIT_RATE[bit_rate] / float(np.max(VIDEO_BIT_RATE))  # last quality
                    if regret_succeed == 1:
                        if latest_chunk_updated:
                            state[0, -1] = VIDEO_BIT_RATE[bit_rate] / float(np.max(VIDEO_BIT_RATE))  # last quality
                    state[2, -1] = float(video_sent_size) / float(delay) / M_IN_K/T_NORM  # kilo byte / ms
                    state[3, -1] = float(delay) / M_IN_K / BUFFER_NORM_FACTOR  # 10 sec

                    state[1, -1] = buffer_size / BUFFER_NORM_FACTOR  # 10 sec
                    state[4, :QUALITY_DIM] = np.array(next_video_chunk_sizes) / M_IN_K / M_IN_K  # mega byte
                    state[5, -1] = np.minimum(video_chunk_remain, CHUNK_TIL_VIDEO_END_CAP) / float(
                        CHUNK_TIL_VIDEO_END_CAP)
                    state[6, :REGRET_WINDOW_SIZE] = np.array(
                        regret_chunk_scalable)   # the times that each chunk has been upgraded
                    state[7, :REGRET_WINDOW_SIZE] = np.array(VIDEO_BIT_RATE)[np.array(
                        regret_chunk_bitrate)] / float(
                        np.max(VIDEO_BIT_RATE))  # exiting quality
                    for i in range(current_buffer_num,REGRET_WINDOW_SIZE):
                        state[7,i]=2


            else:

                # -- reward --
                # reward is video quality - rebuffer penalty - smoothness
                log_bit_rate = np.log2(VIDEO_BIT_RATE[bit_rate] / float(VIDEO_BIT_RATE[0]))
                log_last_bit_rate = np.log2(VIDEO_BIT_RATE[last_bit_rate] / float(VIDEO_BIT_RATE[0]))
                reward = log_bit_rate \
                         - REBUF_PENALTY * rebuf \
                         - SMOOTH_PENALTY * np.abs(log_bit_rate -
                                                   log_last_bit_rate)*(float(max(VIDEO_BIT_RATE[bit_rate],VIDEO_BIT_RATE[last_bit_rate]))/min(VIDEO_BIT_RATE[bit_rate],VIDEO_BIT_RATE[last_bit_rate]))


                r_batch.append(reward)
                r_batch_lable.append(0)
                r_for_train_batch.append(reward)
                last_bit_rate = bit_rate

                # retrieve previous state
                if len(s_batch) == 0:
                    state = [np.zeros((FEATURE_DIM, REGRET_WINDOW_SIZE))]
                else:
                    state = np.array(s_batch[-1], copy=True)

                # dequeue history record
                state = np.roll(state, -1, axis=1)

                # this should be FEATURE_DIM number of terms
                state[0, -1] = VIDEO_BIT_RATE[bit_rate] / float(np.max(VIDEO_BIT_RATE))  # last quality
                state[1, -1] = buffer_size / BUFFER_NORM_FACTOR  # 10 sec
                state[2, -1] = float(video_chunk_size) / float(delay) / M_IN_K/T_NORM  # kilo byte / ms
                state[3, -1] = float(delay) / M_IN_K / BUFFER_NORM_FACTOR  # 10 sec
                state[4, :QUALITY_DIM] = np.array(next_video_chunk_sizes) / M_IN_K / M_IN_K  # mega byte
                state[5, -1] = np.minimum(video_chunk_remain, CHUNK_TIL_VIDEO_END_CAP) / float(CHUNK_TIL_VIDEO_END_CAP)
                state[6, :REGRET_WINDOW_SIZE] = np.array(
                    regret_chunk_scalable)  # the times that each chunk has been upgraded
                state[7, :REGRET_WINDOW_SIZE] = np.array(VIDEO_BIT_RATE)[
                                                                             np.array(regret_chunk_bitrate)] / float(
                    np.max(VIDEO_BIT_RATE))  # exiting quality
                for i in range(current_buffer_num,REGRET_WINDOW_SIZE):
                    state[7,i]=2


            #compute a mask / tells which location can be filled

            if current_buffer_num <= REGRET_WINDOW_SIZE:
                fill_range = current_buffer_num
            else:
                fill_range = REGRET_WINDOW_SIZE
            standard_bitrate = regret_chunk_bitrate[fill_range - 1]

            #for chunks that hasn't been upgraded before
            target_index = -1
            target_bitrate = -1
            original_bitrate = -1
            for i in reversed(range(fill_range)):
                if regret_chunk_scalable[i]==0:
                    if current_buffer_num <= REGRET_WINDOW_SIZE and i == 0:
                        continue
                    if i==fill_range-1:
                        if regret_chunk_bitrate[i-1]>regret_chunk_bitrate[i]:
                            target_index=i
                            target_bitrate=regret_chunk_bitrate[i-1]
                            original_bitrate=regret_chunk_bitrate[i]
                            break
                    elif i==0:
                        if regret_chunk_bitrate[i+1] >regret_chunk_bitrate[i]:
                            target_index = i
                            target_bitrate = regret_chunk_bitrate[i +1]
                            original_bitrate = regret_chunk_bitrate[i]
                            break
                    else:
                        if regret_chunk_bitrate[i-1]>regret_chunk_bitrate[i] and regret_chunk_bitrate[i+1]>regret_chunk_bitrate[i]:
                            target_index = i
                            target_bitrate = max(regret_chunk_bitrate[i - 1],regret_chunk_bitrate[i+1])
                            original_bitrate = regret_chunk_bitrate[i]
                            break
                        if regret_chunk_bitrate[i-1]>regret_chunk_bitrate[i] and regret_chunk_bitrate[i+1]<=regret_chunk_bitrate[i]:
                            target_index = i
                            target_bitrate = regret_chunk_bitrate[i - 1]
                            original_bitrate = regret_chunk_bitrate[i]
                            break
                        if regret_chunk_bitrate[i-1]<=regret_chunk_bitrate[i] and regret_chunk_bitrate[i+1]>regret_chunk_bitrate[i]:
                            target_index = i
                            target_bitrate = regret_chunk_bitrate[i + 1]
                            original_bitrate = regret_chunk_bitrate[i]
                            break
            target_index_f = -1
            target_bitrate_f = -1
            original_bitrate_f = -1
            for i in range(fill_range):
                if regret_chunk_scalable[i]==0:
                    if current_buffer_num <= REGRET_WINDOW_SIZE and i == 0:
                        continue
                    if i==fill_range-1:
                        if regret_chunk_bitrate[i-1]>regret_chunk_bitrate[i]:
                            target_index_f=i
                            target_bitrate_f=regret_chunk_bitrate[i-1]
                            original_bitrate_f=regret_chunk_bitrate[i]
                            break
                    elif i==0:
                        if regret_chunk_bitrate[i+1] >regret_chunk_bitrate[i]:
                            target_index_f = i
                            target_bitrate_f = regret_chunk_bitrate[i +1]
                            original_bitrate_f = regret_chunk_bitrate[i]
                            break
                    else:
                        if regret_chunk_bitrate[i-1]>regret_chunk_bitrate[i] and regret_chunk_bitrate[i+1]>regret_chunk_bitrate[i]:
                            target_index_f = i
                            target_bitrate_f = max(regret_chunk_bitrate[i - 1],regret_chunk_bitrate[i+1])
                            original_bitrate_f = regret_chunk_bitrate[i]
                            break
                        if regret_chunk_bitrate[i-1]>regret_chunk_bitrate[i] and regret_chunk_bitrate[i+1]<=regret_chunk_bitrate[i]:
                            target_index_f = i
                            target_bitrate_f = regret_chunk_bitrate[i - 1]
                            original_bitrate_f = regret_chunk_bitrate[i]
                            break
                        if regret_chunk_bitrate[i-1]<=regret_chunk_bitrate[i] and regret_chunk_bitrate[i+1]>regret_chunk_bitrate[i]:
                            target_index_f = i
                            target_bitrate_f = regret_chunk_bitrate[i + 1]
                            original_bitrate_f = regret_chunk_bitrate[i]
                            break

            #for chunk that has been upgraded once
            t_target_index = -1
            t_target_bitrate = -1
            t_original_bitrate = -1
            for i in reversed(range(fill_range)):
                if regret_chunk_scalable[i] == 1:
                    if current_buffer_num <= REGRET_WINDOW_SIZE and i == 0:
                        continue
                    if i == fill_range - 1:
                        if regret_chunk_bitrate[i - 1] > regret_chunk_bitrate[i]:
                            t_target_index = i
                            t_target_bitrate = regret_chunk_bitrate[i - 1]
                            t_original_bitrate = regret_chunk_bitrate[i]
                            break
                    elif i == 0:
                        if regret_chunk_bitrate[i + 1] > regret_chunk_bitrate[i]:
                            t_target_index = i
                            t_target_bitrate = regret_chunk_bitrate[i + 1]
                            t_original_bitrate = regret_chunk_bitrate[i]
                            break
                    else:
                        if regret_chunk_bitrate[i - 1] > regret_chunk_bitrate[i] and regret_chunk_bitrate[i + 1] > \
                                regret_chunk_bitrate[i]:
                            t_target_index = i
                            t_target_bitrate = max(regret_chunk_bitrate[i - 1], regret_chunk_bitrate[i + 1])
                            t_original_bitrate = regret_chunk_bitrate[i]
                            break
                        if regret_chunk_bitrate[i - 1] > regret_chunk_bitrate[i] and regret_chunk_bitrate[i + 1] <= \
                                regret_chunk_bitrate[i]:
                            t_target_index = i
                            t_target_bitrate = regret_chunk_bitrate[i - 1]
                            t_original_bitrate = regret_chunk_bitrate[i]
                            break
                        if regret_chunk_bitrate[i - 1] <= regret_chunk_bitrate[i] and regret_chunk_bitrate[i + 1] > \
                                regret_chunk_bitrate[i]:
                            t_target_index = i
                            t_target_bitrate = regret_chunk_bitrate[i + 1]
                            t_original_bitrate = regret_chunk_bitrate[i]
                            break
            t_target_index_f = -1
            t_target_bitrate_f = -1
            t_original_bitrate_f = -1
            for i in range(fill_range):
                if regret_chunk_scalable[i] == 1:
                    if current_buffer_num <= REGRET_WINDOW_SIZE and i == 0:
                        continue
                    if i == fill_range - 1:
                        if regret_chunk_bitrate[i - 1] > regret_chunk_bitrate[i]:
                            t_target_index_f = i
                            t_target_bitrate_f = regret_chunk_bitrate[i - 1]
                            t_original_bitrate_f = regret_chunk_bitrate[i]
                            break
                    elif i == 0:
                        if regret_chunk_bitrate[i + 1] > regret_chunk_bitrate[i]:
                            t_target_index_f = i
                            t_target_bitrate_f = regret_chunk_bitrate[i + 1]
                            t_original_bitrate_f = regret_chunk_bitrate[i]
                            break
                    else:
                        if regret_chunk_bitrate[i - 1] > regret_chunk_bitrate[i] and regret_chunk_bitrate[i + 1] > \
                                regret_chunk_bitrate[i]:
                            t_target_index_f = i
                            t_target_bitrate_f = max(regret_chunk_bitrate[i - 1], regret_chunk_bitrate[i + 1])
                            t_original_bitrate_f = regret_chunk_bitrate[i]
                            break
                        if regret_chunk_bitrate[i - 1] > regret_chunk_bitrate[i] and regret_chunk_bitrate[i + 1] <= \
                                regret_chunk_bitrate[i]:
                            t_target_index_f = i
                            t_target_bitrate_f = regret_chunk_bitrate[i - 1]
                            t_original_bitrate_f = regret_chunk_bitrate[i]
                            break
                        if regret_chunk_bitrate[i - 1] <= regret_chunk_bitrate[i] and regret_chunk_bitrate[i + 1] > \
                                regret_chunk_bitrate[i]:
                            t_target_index_f = i
                            t_target_bitrate_f = regret_chunk_bitrate[i + 1]
                            t_original_bitrate_f = regret_chunk_bitrate[i]
                            break


            #target_bitrate = original_bitrate + 1
            #target_bitrate_f = original_bitrate_f + 1
            #t_target_bitrate = t_original_bitrate + 1
            #t_target_bitrate_f = t_original_bitrate_f + 1
            if download_complete:
                action_l = OUTPUT_DIM -3 #default action is all segments have been buffered

                if target_index<t_target_index:
                    target_index=t_target_index
                    target_bitrate=t_target_bitrate
                    action_l = OUTPUT_DIM - 1
                if target_index == -1:  # no more segments to upgrade, tell the simulator to finish the playing back
                    net_env.get_video_chunk(0, 0, 1)
                    play_complete = 1
                else:
                    bit_rate = target_bitrate
                    action_next_or_fill = target_index + 1
                    action = action_l #action_next_or_fill + 5

            else:
                action_mask = net.compute_mask(current_buffer_num,regret_chunk_bitrate)
                # print("action_mask ", action_mask)
                if t_target_index_f==-1: action_mask[0][-2]=0
                if t_target_index==-1: action_mask[0][-1]=0
                if target_index_f==-1: action_mask[0][-4]=0
                if target_index==-1: action_mask[0][-3]=0

                for i in range(1,current_buffer_num):
                    if regret_chunk_scalable[i]>=2:
                        action_mask[0][6+i-1]=0

                action_prob = actor.predict(np.reshape(state, (1, FEATURE_DIM, REGRET_WINDOW_SIZE)))
                # print("action_prob1 is", action_prob)
                # print("action_mask is ", action_mask)
                action_prob = np.multiply(action_prob,action_mask)
                # print("action_prob2 is", action_prob)
                ss = np.sum(action_prob)
                if ss==0:
                    action_prob +=0.1
                    action_prob = np.multiply(action_prob,action_mask)
                    ss = np.sum(action_prob)
                action_prob = action_prob/ss
                action_cumsum = np.cumsum(action_prob)
                # print("action_cumsum ", action_cumsum)
                tmp1=np.random.randint(1, RAND_RANGE) / float(RAND_RANGE)
                # print("tmp1, ", tmp1)
                tmp = (action_cumsum > tmp1)
                #print("tmp:",tmp)
                action = tmp.argmax()#(action_cumsum > np.random.randint(1, RAND_RANGE) / float(RAND_RANGE)).argmax()
                # print("tmp: ", tmp)
                # print("action is ", action)
                # print("OUTPUT_DIM is ", OUTPUT_DIM)
                entropy_record.append(net.compute_entropy(action_prob[0]))
                if action == OUTPUT_DIM-1:
                    bit_rate = t_target_bitrate
                    action_next_or_fill = t_target_index + 1
                elif action == OUTPUT_DIM-2:
                    bit_rate = t_target_bitrate_f
                    action_next_or_fill = t_target_index_f + 1
                elif action == OUTPUT_DIM-3:
                    bit_rate = target_bitrate
                    action_next_or_fill = target_index + 1
                elif action == OUTPUT_DIM-4:
                    bit_rate = target_bitrate_f
                    action_next_or_fill = target_index_f + 1
                elif action <QUALITY_DIM:
                    bit_rate = action
                    action_next_or_fill = 0
                else: # upgrade the quality by one level
                    bit_rate = regret_chunk_bitrate[action - QUALITY_DIM +1] + 1
                    action_next_or_fill = action - QUALITY_DIM+2
            # print("bitrate is ", bit_rate)
            # log time_stamp, bit_rate, buffer_size, reward
            if not play_complete:
                log_file.write(str(time_stamp) + '\t' +
                                'next_action:'+str(action) + '\t' +
                               str(VIDEO_BIT_RATE[bit_rate]) + '\t' +
                               str(round(buffer_size,2)) + '\t' +
                               str(round(rebuf,2)) + '\t' +
                               str(video_chunk_size) + '\t' +
                               str(round(delay,2)) + '\t' +
                               str(round(reward,2)) + '\n')
                log_file.flush()
            else:
                log_file.write(str(time_stamp) + '\t' +
                               str(VIDEO_BIT_RATE[bit_rate]) + '\t' +
                               str(round(buffer_size,2)) + '\t' +
                               str(round(rebuf,2)) + '\t' +
                               str(video_chunk_size) + '\t' +
                               str(round(delay,2)) + '\t' +
                               str(round(reward,2)) + '\n')
                log_file.flush()
            # report experience to the coordinator
            if play_complete:
                exp_queue.put([s_batch[1:],  # ignore the first chuck
                               a_batch[1:],  # since we don't have the
                               r_batch[1:],  # control over it
                               r_batch_lable[1:],
                               r_for_train_batch[1:],
                               play_complete,
                               {'entropy': entropy_record},
                               last_fills_count])
                actor_net_params, critic_net_params = net_params_queue.get()
                actor.set_network_params(actor_net_params)
                critic.set_network_params(critic_net_params)

                del s_batch[:]
                del a_batch[:]
                del r_batch[:]
                del r_batch_lable[:]
                del r_for_train_batch[:]
                del entropy_record[:]

                last_fills_count=0

                log_file.write('\n')  # so that in the log we know where video ends

            # store the state and action into batches
            if play_complete:
                #print("end_of_video")
                last_bit_rate = DEFAULT_QUALITY
                bit_rate = DEFAULT_QUALITY  # use the default action here
                action_next_or_fill = 0


                action_vec = np.zeros(OUTPUT_DIM)
                action_vec[bit_rate] = 1

                s_batch.append(np.zeros((FEATURE_DIM, REGRET_WINDOW_SIZE)))
                a_batch.append(action_vec)

            else:
                s_batch.append(state)

                action_vec = np.zeros(OUTPUT_DIM)
                action_vec[action] = 1
                a_batch.append(action_vec)


def main():

    np.random.seed(RANDOM_SEED)
    assert len(VIDEO_BIT_RATE) == QUALITY_DIM

    # create result directory
    if not os.path.exists(RESULTS_DIR):
        os.makedirs(RESULTS_DIR)

    # inter-process communication queues
    net_params_queues = []
    exp_queues = []
    for i in range(NUM_AGENTS):
        net_params_queues.append(mp.Queue(1))
        exp_queues.append(mp.Queue(1))

    # create a coordinator and multiple agent processes
    # (note: threading is not desirable due to python GIL)
    coordinator = mp.Process(target=central_agent,
                             args=(net_params_queues, exp_queues))
    coordinator.start()

    all_cooked_time, all_cooked_bw, all_file_names = common.load_trace(TRAIN_TRACES)
    agents = []
    for i in range(NUM_AGENTS):
        agents.append(mp.Process(target=agent,
                                 args=(i, all_cooked_time, all_cooked_bw, all_file_names,
                                       net_params_queues[i],
                                       exp_queues[i])))
    for i in range(NUM_AGENTS):
        agents[i].start()

    # wait unit training is done
    coordinator.join()


if __name__ == '__main__':
    main()
