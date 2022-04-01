import os
import sys
os.environ['CUDA_VISIBLE_DEVICES']=''
import numpy as np
import tensorflow as tf
import tensorflow.compat.v1 as tf
tf.disable_v2_behavior()
import common
import net#2 as net
import simulator as env
from config import *

TEST_MODEL = sys.argv[1]
NET_TRACES = sys.argv[2]
USE_REAL_VIDEO_INFO = int(sys.argv[3])



def main():

    assert len(VIDEO_BIT_RATE) == QUALITY_DIM
    np.random.seed(RANDOM_SEED)

    all_cooked_time, all_cooked_bw, all_file_names = common.load_trace(NET_TRACES)

    if USE_REAL_VIDEO_INFO>0: enable_real_video = True
    else: enable_real_video = False

    os.system("rm -r "+TEST_LOG_FOLDER)
    os.system("mkdir "+TEST_LOG_FOLDER)

    net_env = env.Environment(all_cooked_time=all_cooked_time,
                              all_cooked_bw=all_cooked_bw,all_file_names =all_file_names,
                              log_folder = TEST_LOG_FOLDER,
                              use_real_video_info = enable_real_video,
                              mode = TEST)

    log_path = TEST_LOG_FILE + '_' + all_file_names[net_env.trace_idx]
    log_file = open(log_path, 'w')

    with tf.Session() as sess:

        actor = net.ActorNetwork(sess,
                                 state_dim=[FEATURE_DIM, REGRET_WINDOW_SIZE], action_dim=OUTPUT_DIM,
                                 learning_rate=ACTOR_LR_RATE)

        critic = net.CriticNetwork(sess,
                                   state_dim=[FEATURE_DIM, REGRET_WINDOW_SIZE],
                                   learning_rate=CRITIC_LR_RATE)

        sess.run(tf.global_variables_initializer())
        saver = tf.train.Saver()  # save neural net parameters

        # restore neural net parameters
        if TEST_MODEL is not None:  # TEST_MODEL is the path to file
            saver.restore(sess, TEST_MODEL)
            print("Testing model restored.")

        last_bit_rate = DEFAULT_QUALITY
        bit_rate = DEFAULT_QUALITY
        action_next_or_fill = 0
        action_vec = np.zeros(OUTPUT_DIM)
        action_vec[bit_rate] = 1

        s_batch = [np.zeros((FEATURE_DIM, REGRET_WINDOW_SIZE))]
        a_batch = [action_vec]
        r_batch = []
        r_batch_lable = []
        r_for_train_batch = []
        entropy_record = []

        time_stamp = 0
        video_count=0
        while True:
            delay, sleep_time, buffer_size, rebuf, \
            video_chunk_size, next_video_chunk_sizes, \
            download_complete, play_complete, video_chunk_remain, regret_chunk_remain_time, regret_chunk_bitrate, regret_chunk_alternate_size, \
            video_sent_size, \
            regret_succeed, old_bit, last_bit, next_bit, latest_chunk_updated, current_buffer_num, regret_chunk_scalable = \
                net_env.get_video_chunk(bit_rate, action_next_or_fill)  # action_next_or_regret
            log_bit = bit_rate

            '''
            print(next_video_chunk_sizes)

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

            if regret_succeed != 0:
                reward = 0
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
                    log_file.write("fill fail!\n")
                else:
                    log_file.write("fill:" + str(VIDEO_BIT_RATE[old_bit]) + " -> " + str(
                        VIDEO_BIT_RATE[bit_rate]) + '\t' + "actual reward:" + str(reward) + '\n')

                r_batch.append(reward)
                r_batch_lable.append(1)  # 1 means replacement reward
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
                    state[0, -1] = state[
                        0, -2]  # VIDEO_BIT_RATE[bit_rate] / float(np.max(VIDEO_BIT_RATE))  # last quality
                    if regret_succeed == 1:
                        if latest_chunk_updated:
                            state[0, -1] = VIDEO_BIT_RATE[bit_rate] / float(np.max(VIDEO_BIT_RATE))  # last quality
                    state[2, -1] = float(video_sent_size) / float(delay) / M_IN_K / T_NORM  # kilo byte / ms
                    state[3, -1] = float(delay) / M_IN_K / BUFFER_NORM_FACTOR  # 10 sec

                    state[1, -1] = buffer_size / BUFFER_NORM_FACTOR  # 10 sec
                    state[4, :QUALITY_DIM] = np.array(next_video_chunk_sizes) / M_IN_K / M_IN_K  # mega byte
                    state[5, -1] = np.minimum(video_chunk_remain, CHUNK_TIL_VIDEO_END_CAP) / float(
                        CHUNK_TIL_VIDEO_END_CAP)
                    state[6, :REGRET_WINDOW_SIZE] = np.array(
                        regret_chunk_scalable)  # the times that each chunk has been upgraded
                    state[7, :REGRET_WINDOW_SIZE] = np.array(VIDEO_BIT_RATE)[np.array(
                        regret_chunk_bitrate)] / float(
                        np.max(VIDEO_BIT_RATE))  # exiting quality
                    for i in range(current_buffer_num, REGRET_WINDOW_SIZE):
                        state[7, i] = 2


            else:
                # -- reward --
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
                state[2, -1] = float(video_chunk_size) / float(delay) / M_IN_K / T_NORM  # kilo byte / ms
                state[3, -1] = float(delay) / M_IN_K / BUFFER_NORM_FACTOR  # 10 sec
                state[4, :QUALITY_DIM] = np.array(next_video_chunk_sizes) / M_IN_K / M_IN_K  # mega byte
                state[5, -1] = np.minimum(video_chunk_remain, CHUNK_TIL_VIDEO_END_CAP) / float(CHUNK_TIL_VIDEO_END_CAP)
                state[6, :REGRET_WINDOW_SIZE] = np.array(
                    regret_chunk_scalable)  # the times that each chunk has been upgraded
                state[7, :REGRET_WINDOW_SIZE] = np.array(VIDEO_BIT_RATE)[
                                                    np.array(regret_chunk_bitrate)] / float(
                    np.max(VIDEO_BIT_RATE))  # exiting quality
                for i in range(current_buffer_num, REGRET_WINDOW_SIZE):
                    state[7, i] = 2

            # compute a mask / judge which location can be filled

            if current_buffer_num <= REGRET_WINDOW_SIZE:
                fill_range = current_buffer_num
            else:
                fill_range = REGRET_WINDOW_SIZE

            # for chunks that hasn't been upgraded before
            target_index = -1
            target_bitrate = -1
            original_bitrate = -1
            for i in reversed(range(fill_range)):
                if regret_chunk_scalable[i] == 0:
                    if current_buffer_num <= REGRET_WINDOW_SIZE and i == 0:
                        continue
                    if i == fill_range - 1:
                        if regret_chunk_bitrate[i - 1] > regret_chunk_bitrate[i]:
                            target_index = i
                            target_bitrate = regret_chunk_bitrate[i - 1]
                            original_bitrate = regret_chunk_bitrate[i]
                            break
                    elif i == 0:
                        if regret_chunk_bitrate[i + 1] > regret_chunk_bitrate[i]:
                            target_index = i
                            target_bitrate = regret_chunk_bitrate[i + 1]
                            original_bitrate = regret_chunk_bitrate[i]
                            break
                    else:
                        if regret_chunk_bitrate[i - 1] > regret_chunk_bitrate[i] and regret_chunk_bitrate[i + 1] > \
                                regret_chunk_bitrate[i]:
                            target_index = i
                            target_bitrate = max(regret_chunk_bitrate[i - 1], regret_chunk_bitrate[i + 1])
                            original_bitrate = regret_chunk_bitrate[i]
                            break
                        if regret_chunk_bitrate[i - 1] > regret_chunk_bitrate[i] and regret_chunk_bitrate[i + 1] <= \
                                regret_chunk_bitrate[i]:
                            target_index = i
                            target_bitrate = regret_chunk_bitrate[i - 1]
                            original_bitrate = regret_chunk_bitrate[i]
                            break
                        if regret_chunk_bitrate[i - 1] <= regret_chunk_bitrate[i] and regret_chunk_bitrate[i + 1] > \
                                regret_chunk_bitrate[i]:
                            target_index = i
                            target_bitrate = regret_chunk_bitrate[i + 1]
                            original_bitrate = regret_chunk_bitrate[i]
                            break
            target_index_f = -1
            target_bitrate_f = -1
            original_bitrate_f = -1
            for i in range(fill_range):
                if regret_chunk_scalable[i] == 0:
                    if current_buffer_num <= REGRET_WINDOW_SIZE and i == 0:
                        continue
                    if i == fill_range - 1:
                        if regret_chunk_bitrate[i - 1] > regret_chunk_bitrate[i]:
                            target_index_f = i
                            target_bitrate_f = regret_chunk_bitrate[i - 1]
                            original_bitrate_f = regret_chunk_bitrate[i]
                            break
                    elif i == 0:
                        if regret_chunk_bitrate[i + 1] > regret_chunk_bitrate[i]:
                            target_index_f = i
                            target_bitrate_f = regret_chunk_bitrate[i + 1]
                            original_bitrate_f = regret_chunk_bitrate[i]
                            break
                    else:
                        if regret_chunk_bitrate[i - 1] > regret_chunk_bitrate[i] and regret_chunk_bitrate[i + 1] > \
                                regret_chunk_bitrate[i]:
                            target_index_f = i
                            target_bitrate_f = max(regret_chunk_bitrate[i - 1], regret_chunk_bitrate[i + 1])
                            original_bitrate_f = regret_chunk_bitrate[i]
                            break
                        if regret_chunk_bitrate[i - 1] > regret_chunk_bitrate[i] and regret_chunk_bitrate[i + 1] <= \
                                regret_chunk_bitrate[i]:
                            target_index_f = i
                            target_bitrate_f = regret_chunk_bitrate[i - 1]
                            original_bitrate_f = regret_chunk_bitrate[i]
                            break
                        if regret_chunk_bitrate[i - 1] <= regret_chunk_bitrate[i] and regret_chunk_bitrate[i + 1] > \
                                regret_chunk_bitrate[i]:
                            target_index_f = i
                            target_bitrate_f = regret_chunk_bitrate[i + 1]
                            original_bitrate_f = regret_chunk_bitrate[i]
                            break

            # for chunk that has been upgraded once
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
                action_l = OUTPUT_DIM - 3

                if target_index < t_target_index:
                    target_index = t_target_index
                    target_bitrate = t_target_bitrate
                    action_l = OUTPUT_DIM - 1
                if target_index == -1:
                    net_env.get_video_chunk(0, 0, 1)
                    play_complete = 1
                else:
                    bit_rate = target_bitrate
                    action_next_or_fill = target_index + 1
                    action = action_l  # action_next_or_fill + 5


            else:
                action_mask = net.compute_mask(current_buffer_num, regret_chunk_bitrate)

                if t_target_index_f == -1: action_mask[0][-2] = 0
                if t_target_index == -1: action_mask[0][-1] = 0
                if target_index_f == -1: action_mask[0][-4] = 0
                if target_index == -1: action_mask[0][-3] = 0

                for i in range(1,current_buffer_num):
                    if regret_chunk_scalable[i]>=2:
                        action_mask[0][6+i-1]=0

                action_prob = actor.predict(np.reshape(state, (1, FEATURE_DIM, REGRET_WINDOW_SIZE)))

                action_prob = np.multiply(action_prob, action_mask)
                ss = np.sum(action_prob)
                if ss==0:
                    action_prob +=0.1
                    action_prob = np.multiply(action_prob,action_mask)
                    ss = np.sum(action_prob)
                action_prob = action_prob/ss
                action_cumsum = np.cumsum(action_prob)

                tmp1 = np.random.randint(1, RAND_RANGE) / float(RAND_RANGE)
                tmp = (action_cumsum > tmp1)
                action = tmp.argmax()  # (action_cumsum > np.random.randint(1, RAND_RANGE) / float(RAND_RANGE)).argmax()

                entropy_record.append(net.compute_entropy(action_prob[0]))
                if action == OUTPUT_DIM - 1:
                    bit_rate = t_target_bitrate
                    action_next_or_fill = t_target_index + 1
                elif action == OUTPUT_DIM - 2:
                    bit_rate = t_target_bitrate_f
                    action_next_or_fill = t_target_index_f + 1
                elif action == OUTPUT_DIM - 3:
                    bit_rate = target_bitrate
                    action_next_or_fill = target_index + 1
                elif action == OUTPUT_DIM - 4:
                    bit_rate = target_bitrate_f
                    action_next_or_fill = target_index_f + 1
                elif action < QUALITY_DIM:
                    bit_rate = action
                    action_next_or_fill = 0
                else:
                    bit_rate = regret_chunk_bitrate[action - QUALITY_DIM + 1] + 1
                    action_next_or_fill = action - QUALITY_DIM + 2
            # log time_stamp, bit_rate, buffer_size, reward
            if play_complete:
                log_file.write(str(time_stamp) + '\t' +
                               str(VIDEO_BIT_RATE[log_bit]) + '\t' +
                               str(round(buffer_size,2)) + '\t' +
                               str(round(rebuf,2)) + '\t' +
                               str(round(video_chunk_size,2)) + '\t' +
                               str(round(delay,2)) + '\t' +
                               str(round(reward,2)) + '\n')
                log_file.flush()
            else:
                log_file.write(str(time_stamp) + '\t' +
                               str(VIDEO_BIT_RATE[log_bit]) + '\t' +
                               str(round(buffer_size,2)) + '\t' +
                               str(round(rebuf,2)) + '\t' +
                               str(round(video_chunk_size,2)) + '\t' +
                               str(round(delay,2)) + '\t' +
                               str(round(reward,2)) + '\t'+"next_to_replace?:" + str(action_next_or_fill)+ 'next_action:'+str(action)+'\n')
                log_file.flush()

                if action_next_or_fill !=0:
                    log_file.write(str(regret_chunk_bitrate)+"\t")
                    log_file.flush()

            if play_complete:
                log_file.write('\n')
                log_file.close()

                last_bit_rate = DEFAULT_QUALITY
                bit_rate = DEFAULT_QUALITY  # use the default action here
                action_next_or_fill = 0

                del s_batch[:]
                del a_batch[:]
                del r_batch[:]
                del r_for_train_batch[:]
                del entropy_record[:]

                action_vec = np.zeros(OUTPUT_DIM)
                action_vec[bit_rate] = 1

                s_batch.append(np.zeros((FEATURE_DIM, REGRET_WINDOW_SIZE)))
                a_batch.append(action_vec)
                entropy_record = []

                video_count += 1

                if video_count >= len(all_file_names):
                    print("exsiting rl_test!")
                    break

                log_path = TEST_LOG_FILE + '_' + all_file_names[net_env.trace_idx]
                log_file = open(log_path, 'w')
            else:
                s_batch.append(state)
                action_vec = np.zeros(OUTPUT_DIM)
                action_vec[action] = 1
                a_batch.append(action_vec)


if __name__ == '__main__':
    main()
