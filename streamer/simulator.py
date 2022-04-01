import numpy as np
import os



#####fixed param
MILLISECONDS_IN_SECOND = 1000.0
B_IN_MB = 1000000.0
BITS_IN_BYTE = 8.0
RANDOM_SEED = 42
RAND_RANGE = 1000

#####task setting
from config import *
TOTAL_VIDEO_CHUNCK = int(CHUNK_TIL_VIDEO_END_CAP)

#####simulator setting
VIDEO_CHUNCK_LEN = VIDEO_CHUNCK_SIZE * MILLISECONDS_IN_SECOND  # millisec, every time add this amount to buffer
BUFFER_THRESH = BUFFER_THRESH_SIZE * MILLISECONDS_IN_SECOND  # millisec, max buffer limit
DRAIN_BUFFER_SLEEP_TIME = 500.0  # millisec
PACKET_PAYLOAD_PORTION = 0.95
LINK_RTT = 80  # millisec
PACKET_SIZE = 1500  # bytes
NOISE_LOW = 0.8
NOISE_HIGH = 1.0
SVC_OVERHEAD_RANGE = [[0.15,0.2],[0.3,0.4]]


def get_real_chunk_size(replace_type=None):
    readin=open(REAL_CHUNK_SIZE_LOG,"r")
    video_info = {}
    for line in readin.readlines():
        parse=line.split()
        if len(parse)<3:continue
        chunk_index=eval(parse[0])
        key=parse[1]
        bitrate=eval(parse[2])
        if chunk_index not in video_info.keys():
            video_info[chunk_index]={}
        video_info[chunk_index][key]=bitrate*VIDEO_CHUNCK_SIZE*1000.0/8.0

    if replace_type:
        for chunk_index in video_info.keys():
            for el_type in video_info[chunk_index].keys():
                if el_type in replace_type.keys():
                    video_info[chunk_index][el_type]=video_info[chunk_index][replace_type[el_type]]
    return video_info


def get_simulated_chunk_size():
    noise_range = [0.9, 1.1]
    video_info = {}

    for bitrate in range(QUALITY_DIM):
        video_info[bitrate] = []
        for i in range(TOTAL_VIDEO_CHUNCK):
            noise_ratio = np.random.randint(int(noise_range[0] * RAND_RANGE),
                              int(noise_range[1] * RAND_RANGE)) / float(RAND_RANGE)
            video_info[bitrate].append(VIDEO_BIT_RATE[bitrate]*1000.0*VIDEO_CHUNCK_SIZE*noise_ratio/BITS_IN_BYTE)
    return video_info




class Environment:
    def __init__(self, all_cooked_time, all_cooked_bw,all_file_names,
                 log_folder,
                 use_real_video_info = False,
                 mode= TRAIN,
                 random_seed=RANDOM_SEED):
        assert len(all_cooked_time) == len(all_cooked_bw)

        np.random.seed(random_seed)
        self.all_file_names = all_file_names
        self.all_cooked_time = all_cooked_time
        self.all_cooked_bw = all_cooked_bw
        self.mode = mode
        self.log_folder = log_folder
        self.real_video_enabled = use_real_video_info

        self.video_chunk_counter = 0
        self.buffer_size = 0

        self.download_complete = False

        self.buffer_chunk_num = 0
        self.buffer_chunk_alternable_size = []
        self.buffer_chunk_bitrate = []
        self.buffer_chunk_scalable = []
        self.buffer_chunk_remain_time = []
        self.buffer_chunk_size = []

        self.buffer_chunk_rebuff = []
        self.buffer_chunk_last_bitrate = []
        self.update=[]

        # pick a trace file
        if self.mode==TRAIN:
            self.trace_idx = np.random.randint(len(self.all_cooked_time))
        else:
            self.trace_idx = 0
        self.cooked_time = self.all_cooked_time[self.trace_idx]
        self.cooked_bw = self.all_cooked_bw[self.trace_idx]

        if self.mode==TRAIN:
            self.mahimahi_ptr = np.random.randint(1, len(self.cooked_bw))
        else:
            self.mahimahi_ptr = 1
        self.last_mahimahi_time = self.cooked_time[self.mahimahi_ptr - 1]

        # create the played back quality log file
        #os.system("rm -r " + self.log_folder + "/playback_quality/")
        os.system("mkdir " + self.log_folder + "/playback_quality/")
        log_path = self.log_folder + "/playback_quality/" +'/log_playback_' + self.all_file_names[self.trace_idx]   #for logging played back quality
        self.log_file = open(log_path, 'w')

        #update_log = UPDATE_LOG + '_' + self.all_file_names[self.trace_idx]
        #create the update log file
        #os.system("rm -r "+self.log_folder+"/update_log/")
        os.system("mkdir "+self.log_folder+"/update_log/")
        self.update_log_file = open(self.log_folder+"/update_log/" + 'switch_up_log_file',"w")

        if self.real_video_enabled:
            self.video_size = get_real_chunk_size()#{"23":"22","34":"33","45":"44"})  # in bytes
        else:
            self.video_size = get_simulated_chunk_size()
        print(self.video_size)
    ################################
    # Input
    # quality: the quality to download for the target video chunk
    # next_or_regret: 0 - download next segment | 1~REGRET_WINDOW_SIZE - update the quality of a buffered segment
    # play_to_end: 1 - no more download action, just finish playing the video
    ################################

    ################################
    # Return
    # 1. the time for executing this download    2. buffer overflow sleep time      3. buffered video length
    # 4. rebuffering time                        5. num of bytes to downloaded         6. size of the different quality versions of the next segment
    # 7. if the download has completed           8. if the playing back has completed   9. number of segments remaine to be downloaded
    # 10. the duration of buffered segments      11. the bitrate of buffered segments    12. size of other quality versions of buffered segments
    # 13. num of bytes actually downloaded
    # 14. result flag: 0 - next segment downloaded | 1 - quality upgrade finished | 2 - quality upgrade deadline exceeded (the segment is played back before download can finish)
    # 15. the existing bitrate of the target segment (0 if not buffered)            16. bitrate of the previous segment of the target
    # 17. bitrate of the next segment of the target      18. if the target segment is the latest buffered segment
    # 19. num of segments in buffer         20. the times that the quality of each buffered segment has been upgraded
    ################################
    def get_video_chunk(self, quality, next_or_regret, play_to_end=0):

        assert quality >= 0
        assert quality < QUALITY_DIM

        assert next_or_regret >= 0 and next_or_regret <= REGRET_WINDOW_SIZE
        regret_succeed = 2

        assert next_or_regret <= self.buffer_chunk_num or next_or_regret == 0

        if play_to_end:
            self.buffer_size = 0
            self.video_chunk_counter = 0
            self.download_complete = False

            #if self.mode ==TEST:
            for chunk in self.buffer_chunk_bitrate:
                self.log_file.write(str(chunk)+" ")
            for h in self.update:
                for b in h:
                    self.update_log_file.write(str(b)+" ")
                self.update_log_file.write("\n")
            self.log_file.write("\n")

            self.buffer_chunk_num = 0
            self.buffer_chunk_remain_time = []
            self.buffer_chunk_scalable = []
            self.buffer_chunk_alternable_size = []
            self.buffer_chunk_bitrate = []
            self.buffer_chunk_rebuff = []
            self.buffer_chunk_last_bitrate = []
            self.buffer_chunk_size = []
            self.update = []
            # pick a random trace file
            if self.mode == TRAIN:
                self.trace_idx = np.random.randint(len(self.all_cooked_time))
            else:
                self.trace_idx += 1
                if self.trace_idx >= len(self.all_cooked_time):
                    self.trace_idx = 0

            self.cooked_time = self.all_cooked_time[self.trace_idx]
            self.cooked_bw = self.all_cooked_bw[self.trace_idx]

            # randomize the start point of the video
            # note: trace file starts with time 0
            if self.mode == TRAIN:
                self.mahimahi_ptr = np.random.randint(1, len(self.cooked_bw))
            else:
                self.mahimahi_ptr = 1
            self.last_mahimahi_time = self.cooked_time[self.mahimahi_ptr - 1]

            #if self.mode ==TEST:
            self.log_file.close()
            #if self.trace_idx!=0:
            log_path = self.log_folder + "/playback_quality/" +'/log_playback_' + self.all_file_names[self.trace_idx]   #for logging played back quality
            self.log_file = open(log_path, 'w')
            return 0, \
                   0, \
                   0, \
                   0, \
                   0, \
                   0, \
                   True, \
                   True, \
                   0, \
                   0, 0, 0, \
                   0, \
                   0, \
                   0, \
                   None, None, 0, 0

        if next_or_regret == 0:
            regret_succeed = 0
            if self.real_video_enabled:
                video_chunk_size = self.video_size[self.video_chunk_counter][str(quality)]
            else:
                video_chunk_size = self.video_size[quality][self.video_chunk_counter]
            # use the delivery opportunity in mahimahi
            delay = 0.0  # in ms
            video_chunk_counter_sent = 0  # in bytes

            while True:  # download video chunk over mahimahi
                throughput = self.cooked_bw[self.mahimahi_ptr] \
                             * B_IN_MB / BITS_IN_BYTE
                duration = self.cooked_time[self.mahimahi_ptr] \
                           - self.last_mahimahi_time

                packet_payload = throughput * duration * PACKET_PAYLOAD_PORTION

                if video_chunk_counter_sent + packet_payload > video_chunk_size:
                    fractional_time = (video_chunk_size - video_chunk_counter_sent) / \
                                      throughput / PACKET_PAYLOAD_PORTION
                    delay += fractional_time
                    self.last_mahimahi_time += fractional_time
                    assert (self.last_mahimahi_time <= self.cooked_time[self.mahimahi_ptr])
                    video_chunk_counter_sent = video_chunk_size
                    break

                video_chunk_counter_sent += packet_payload
                # print("druation:",duration)
                delay += duration
                self.last_mahimahi_time = self.cooked_time[self.mahimahi_ptr]
                self.mahimahi_ptr += 1

                if self.mahimahi_ptr >= len(self.cooked_bw):
                    # loop back in the beginning
                    # note: trace file starts with time 0
                    self.mahimahi_ptr = 1
                    self.last_mahimahi_time = 0

            delay *= MILLISECONDS_IN_SECOND
            delay += LINK_RTT

            # add a multiplicative noise to the delay
            if self.mode == TRAIN:
                delay *= np.random.uniform(NOISE_LOW, NOISE_HIGH)
            return_delay = delay

            # rebuffer time
            rebuf = np.maximum(delay - self.buffer_size, 0.0)

            # update the buffer
            if self.buffer_size - delay < 0:
                self.buffer_size = 0.0
                self.buffer_chunk_num = 0
                self.buffer_chunk_alternable_size = []

                #if self.mode == TEST:
                for chunk in self.buffer_chunk_bitrate:
                    self.log_file.write(str(chunk)+" ")
                for h in self.update:
                    for b in h:
                        self.update_log_file.write(str(b)+" ")
                    self.update_log_file.write("\n")
                self.update = []
                self.buffer_chunk_bitrate = []
                self.buffer_chunk_scalable = []
                self.buffer_chunk_remain_time = []
                self.buffer_chunk_rebuff = []
                self.buffer_chunk_last_bitrate = []
                self.buffer_chunk_size = []
            else:
                self.buffer_size -= delay
                for i in range(self.buffer_chunk_num):
                    delay -= self.buffer_chunk_remain_time[0]
                    if delay < 0:
                        self.buffer_chunk_remain_time[0] = -delay
                        break
                    else:
                        self.buffer_chunk_remain_time.pop(0)
                        self.buffer_chunk_last_bitrate.pop(0)
                        self.buffer_chunk_rebuff.pop(0)
                        self.buffer_chunk_num -= 1
                        #if self.mode ==TEST:
                        self.log_file.write(str(self.buffer_chunk_bitrate[0])+" ")
                        for b in self.update[0]:
                            self.update_log_file.write(str(b)+" ")
                        self.update_log_file.write("\n")
                        self.update.pop(0)

                        self.buffer_chunk_bitrate.pop(0)
                        self.buffer_chunk_scalable.pop(0)
                        self.buffer_chunk_alternable_size.pop(0)
                        self.buffer_chunk_size.pop(0)

            # add in the new chunk
            self.buffer_chunk_num += 1
            self.buffer_size += VIDEO_CHUNCK_LEN

            new_video_chunk_sizes = []
            for i in range(QUALITY_DIM):
                if self.real_video_enabled:
                    new_video_chunk_sizes.append(self.video_size[self.video_chunk_counter][str(i)])
                else:
                    new_video_chunk_sizes.append(self.video_size[i][self.video_chunk_counter])

            self.buffer_chunk_bitrate.append(quality)
            self.update.append([quality])
            self.buffer_chunk_scalable.append(0)
            self.buffer_chunk_alternable_size.append(list(new_video_chunk_sizes))
            self.buffer_chunk_remain_time.append(VIDEO_CHUNCK_LEN)
            self.buffer_chunk_size.append(video_chunk_size)
            if len(self.buffer_chunk_bitrate) > 1:
                self.buffer_chunk_last_bitrate.append(self.buffer_chunk_bitrate[-2])
            else:
                self.buffer_chunk_last_bitrate.append(None)
            self.buffer_chunk_rebuff.append(rebuf)

            # sleep if buffer gets too large
            sleep_time = 0
            sleep_return =0
            if self.buffer_size > BUFFER_THRESH:
                # exceed the buffer limit
                # we need to skip some network bandwidth here
                # but do not add up the delay
                drain_buffer_time = self.buffer_size - BUFFER_THRESH
                sleep_time = np.ceil(drain_buffer_time / DRAIN_BUFFER_SLEEP_TIME) * \
                             DRAIN_BUFFER_SLEEP_TIME

                self.buffer_size -= sleep_time

                sleep_time_tmp = sleep_time
                sleep_return = sleep_time
                for i in range(self.buffer_chunk_num):
                    sleep_time_tmp -= self.buffer_chunk_remain_time[0]
                    if sleep_time_tmp < 0:
                        self.buffer_chunk_remain_time[0] = -sleep_time_tmp
                        break
                    else:
                        self.buffer_chunk_remain_time.pop(0)
                        self.buffer_chunk_size.pop(0)
                        self.buffer_chunk_num -= 1

                        #if self.mode ==TEST:
                        self.log_file.write(str(self.buffer_chunk_bitrate[0])+" ")
                        for b in self.update[0]:
                            self.update_log_file.write(str(b)+" ")
                        self.update_log_file.write("\n")
                        self.update.pop(0)

                        self.buffer_chunk_bitrate.pop(0)
                        self.buffer_chunk_scalable.pop(0)
                        self.buffer_chunk_alternable_size.pop(0)
                        self.buffer_chunk_rebuff.pop(0)
                        self.buffer_chunk_last_bitrate.pop(0)

                while True:
                    duration = self.cooked_time[self.mahimahi_ptr] \
                               - self.last_mahimahi_time
                    if duration > sleep_time / MILLISECONDS_IN_SECOND:
                        self.last_mahimahi_time += sleep_time / MILLISECONDS_IN_SECOND
                        break
                    sleep_time -= duration * MILLISECONDS_IN_SECOND
                    self.last_mahimahi_time = self.cooked_time[self.mahimahi_ptr]
                    self.mahimahi_ptr += 1

                    if self.mahimahi_ptr >= len(self.cooked_bw):
                        # loop back in the beginning
                        # note: trace file starts with time 0
                        self.mahimahi_ptr = 1
                        self.last_mahimahi_time = 0

            # the "last buffer size" return to the controller
            # Note: in old version of dash the lowest buffer is 0.
            # In the new version the buffer always have at least
            # one chunk of video
            return_buffer_size = self.buffer_size

            self.video_chunk_counter += 1
            video_chunk_remain = TOTAL_VIDEO_CHUNCK - self.video_chunk_counter

            # chance to fix low quality chunk
            regret_chunk_bitrate = []
            regret_chunk_remain_time = []
            regret_chunk_alternate_size = []
            regret_chunk_scalable = []
            for i in range(REGRET_WINDOW_SIZE):
                if self.buffer_chunk_num - REGRET_WINDOW_SIZE + i < 0:
                    pass
                else:
                    regret_chunk_alternate_size.append(
                        list(self.buffer_chunk_alternable_size[self.buffer_chunk_num - REGRET_WINDOW_SIZE + i]))
                    regret_chunk_scalable.append(
                        self.buffer_chunk_scalable[self.buffer_chunk_num - REGRET_WINDOW_SIZE + i])
                    regret_chunk_bitrate.append(
                        self.buffer_chunk_bitrate[self.buffer_chunk_num - REGRET_WINDOW_SIZE + i])
                    regret_chunk_remain_time.append(self.buffer_chunk_remain_time[
                                                        self.buffer_chunk_num - REGRET_WINDOW_SIZE + i] / MILLISECONDS_IN_SECOND)

            for i in range(REGRET_WINDOW_SIZE - len(regret_chunk_alternate_size)):
                regret_chunk_alternate_size.append([0, 0, 0, 0, 0, 0])
                regret_chunk_bitrate.append(0)
                regret_chunk_remain_time.append(0)
                regret_chunk_scalable.append(0)

            download_complete = False
            play_complete = False
            if self.video_chunk_counter >= TOTAL_VIDEO_CHUNCK:
                download_complete = True
                self.download_complete = True

            next_video_chunk_sizes = []
            for i in range(QUALITY_DIM):
                if not download_complete:
                    if self.real_video_enabled:
                        next_video_chunk_sizes.append(self.video_size[self.video_chunk_counter][str(i)])
                    else:
                        next_video_chunk_sizes.append(self.video_size[i][self.video_chunk_counter])
                else:
                    next_video_chunk_sizes.append(0)

            return return_delay, \
                   sleep_return, \
                   return_buffer_size / MILLISECONDS_IN_SECOND, \
                   rebuf / MILLISECONDS_IN_SECOND, \
                   video_chunk_size, \
                   next_video_chunk_sizes, \
                   download_complete, \
                   play_complete, \
                   video_chunk_remain, \
                   regret_chunk_remain_time, regret_chunk_bitrate, regret_chunk_alternate_size, \
                   video_chunk_counter_sent, \
                   regret_succeed, \
                   0, \
                   None, None, 0, self.buffer_chunk_num, regret_chunk_scalable
        else:
            if self.buffer_chunk_num <= REGRET_WINDOW_SIZE:
                # print("next_or_regret, ", next_or_regret)
                # index = next_or_regret - 1
                index = next_or_regret-1
            else:
                # print("self.buffer_chunk_num, ", self.buffer_chunk_num, "REGRET_WINDOW_SIZE, ", REGRET_WINDOW_SIZE, "next_or_regret, ", next_or_regret)
                index = self.buffer_chunk_num - REGRET_WINDOW_SIZE + next_or_regret - 1

            if index == self.buffer_chunk_num - 1:
                latest_chunk_updated = 1
            else:
                latest_chunk_updated = 0

            if self.real_video_enabled:
                if self.video_size[self.video_chunk_counter - 1 - (self.buffer_chunk_num - index - 1)][str(quality)]!= \
                        self.buffer_chunk_alternable_size[index][quality]:
                    print("env wrong!!!!!!!!!!!!!!!!check again!!!!!!!!!!!!!!!!!!!")
                    print(self.video_chunk_counter - 1 - (self.buffer_chunk_num - index - 1))
                    print("index:", index)
                    print("buffer num", self.buffer_chunk_num)
                    print("self.vide_count", self.video_chunk_counter)
                assert self.video_size[self.video_chunk_counter - 1 - (self.buffer_chunk_num - index - 1)][str(quality)] == \
                       self.buffer_chunk_alternable_size[index][quality]
            else:
                if self.video_size[quality][self.video_chunk_counter - 1 - (self.buffer_chunk_num - index - 1)] != \
                        self.buffer_chunk_alternable_size[index][quality]:
                    print("env wrong!!!!!!!!!!!!!!!!check again!!!!!!!!!!!!!!!!!!!")
                    print(self.video_chunk_counter - 1 - (self.buffer_chunk_num - index - 1))
                    print("index:", index)
                    print("buffer num", self.buffer_chunk_num)
                    print("self.vide_count", self.video_chunk_counter)
                assert self.video_size[quality][self.video_chunk_counter - 1 - (self.buffer_chunk_num - index - 1)] == \
                       self.buffer_chunk_alternable_size[index][quality]

            #assert self.buffer_chunk_scalable[index] == 1
            # print("self.buffer_chunk_scalable", self.buffer_chunk_scalable)
            # print("index is, ", index)
            # assert self.buffer_chunk_scalable[index] < 2

            if self.real_video_enabled:
                size_key=""
                for t in self.update[index]: size_key+=str(t)
                size_key+=str(quality)
                if size_key in self.video_size[self.video_chunk_counter - 1 - (self.buffer_chunk_num - index - 1)].keys():
                    video_chunk_size = self.video_size[
                                    self.video_chunk_counter - 1 - (self.buffer_chunk_num - index - 1)][size_key]-self.video_size[
                                    self.video_chunk_counter - 1 - (self.buffer_chunk_num - index - 1)][size_key[:-1]]
                else:
                    video_chunk_size = self.video_size[
                                    self.video_chunk_counter - 1 - (self.buffer_chunk_num - index - 1)][size_key[-1]]*(1+0.4)-self.video_size[
                                    self.video_chunk_counter - 1 - (self.buffer_chunk_num - index - 1)][size_key[:-1]]
            else:
                # overhead_range = SVC_OVERHEAD_RANGE[self.buffer_chunk_scalable[index]]
                overhead_range = [0.15,0.2]
                overhead_ratio = np.random.randint(int(overhead_range[0] * RAND_RANGE),
                                                   int(overhead_range[1] * RAND_RANGE)) / float(RAND_RANGE)


                video_chunk_size = self.video_size[quality][
                                       self.video_chunk_counter - 1 - (self.buffer_chunk_num - index - 1)] * (
                                               1 + overhead_ratio) - self.buffer_chunk_size[index]

            #self.buffer_chunk_size[index] += video_chunk_size

            #assert int(video_chunk_size)==video_chunk_size
            time_before_chunk = 0
            for i in range(index):
                time_before_chunk += self.buffer_chunk_remain_time[i] / MILLISECONDS_IN_SECOND

            total_time_available = time_before_chunk
            delay = 0.0  # in ms
            video_chunk_counter_sent = 0  # in bytes


            while True:  # download video chunk over mahimahi
                throughput = self.cooked_bw[self.mahimahi_ptr] \
                             * B_IN_MB / BITS_IN_BYTE
                duration = self.cooked_time[self.mahimahi_ptr] \
                           - self.last_mahimahi_time

                if time_before_chunk < duration:
                    duration = time_before_chunk
                    time_before_chunk = 0
                else:
                    time_before_chunk -= duration

                packet_payload = throughput * duration * PACKET_PAYLOAD_PORTION

                if video_chunk_counter_sent + packet_payload > video_chunk_size:
                    fractional_time = (video_chunk_size - video_chunk_counter_sent) / \
                                      throughput / PACKET_PAYLOAD_PORTION
                    video_chunk_counter_sent = video_chunk_size
                    delay += fractional_time
                    self.last_mahimahi_time += fractional_time
                    assert (self.last_mahimahi_time <= self.cooked_time[self.mahimahi_ptr])
                    regret_succeed = 1
                    break

                if duration < self.cooked_time[self.mahimahi_ptr] - self.last_mahimahi_time:
                    self.last_mahimahi_time += duration
                else:
                    # print("druation:",duration)
                    self.last_mahimahi_time = self.cooked_time[self.mahimahi_ptr]
                    self.mahimahi_ptr += 1

                delay += duration
                video_chunk_counter_sent += packet_payload

                if self.mahimahi_ptr >= len(self.cooked_bw):
                    # loop back in the beginning
                    # note: trace file starts with time 0
                    self.mahimahi_ptr = 1
                    self.last_mahimahi_time = 0

                if time_before_chunk <= 0:
                    regret_succeed = 2
                    break

            old_bitrate = self.buffer_chunk_bitrate[index]
            last_bitrate = self.buffer_chunk_last_bitrate[index]
            old_rebuff = self.buffer_chunk_rebuff[index]
            if index < self.buffer_chunk_num - 1:
                next_bitrate = self.buffer_chunk_bitrate[index + 1]
            else:
                next_bitrate = None

            if regret_succeed == 2:
                delay = total_time_available
            else:
                self.buffer_chunk_bitrate[index] = quality
                self.update[index].append(quality)
                self.buffer_chunk_size[index] += video_chunk_size
                # update whether it is scalale
                self.buffer_chunk_scalable[index] = self.buffer_chunk_scalable[index] + 1

                if index < self.buffer_chunk_num - 1:
                    self.buffer_chunk_last_bitrate[index + 1] = quality

            if delay != 0:
                delay *= MILLISECONDS_IN_SECOND
                if regret_succeed != 2:
                    delay += LINK_RTT

            # add a multiplicative noise to the delay
            if regret_succeed != 2:
                delay *= np.random.uniform(NOISE_LOW, NOISE_HIGH)

            return_delay = delay

            if self.buffer_size - delay < 0:
                self.buffer_size = 0.0
                self.buffer_chunk_num = 0
                self.buffer_chunk_alternable_size = []
                for chunk in self.buffer_chunk_bitrate:
                    self.log_file.write(str(chunk)+" ")
                for h in self.update:
                    for b in h:
                        self.update_log_file.write(str(b)+" ")
                    self.update_log_file.write("\n")
                self.update = []
                self.buffer_chunk_bitrate = []
                self.buffer_chunk_scalable = []
                self.buffer_chunk_remain_time = []
                self.buffer_chunk_size = []
            else:
                self.buffer_size -= delay
                for i in range(self.buffer_chunk_num):
                    delay -= self.buffer_chunk_remain_time[0]
                    if delay < 0:
                        self.buffer_chunk_remain_time[0] = -delay
                        break
                    else:
                        self.buffer_chunk_remain_time.pop(0)
                        self.buffer_chunk_num -= 1
                        self.log_file.write(str(self.buffer_chunk_bitrate[0])+" ")
                        for b in self.update[0]:
                            self.update_log_file.write(str(b)+" ")
                        self.update_log_file.write("\n")
                        self.update.pop(0)
                        self.buffer_chunk_bitrate.pop(0)
                        self.buffer_chunk_scalable.pop(0)
                        self.buffer_chunk_alternable_size.pop(0)
                        self.buffer_chunk_rebuff.pop(0)
                        self.buffer_chunk_size.pop(0)
                        self.buffer_chunk_last_bitrate.pop(0)

            # chance to fix low quality chunk
            regret_chunk_bitrate = []
            regret_chunk_remain_time = []
            regret_chunk_alternate_size = []
            regret_chunk_scalable = []
            for i in range(REGRET_WINDOW_SIZE):
                if self.buffer_chunk_num - REGRET_WINDOW_SIZE + i < 0:
                    pass
                else:
                    regret_chunk_alternate_size.append(
                        list(self.buffer_chunk_alternable_size[self.buffer_chunk_num - REGRET_WINDOW_SIZE + i]))
                    regret_chunk_bitrate.append(
                        self.buffer_chunk_bitrate[self.buffer_chunk_num - REGRET_WINDOW_SIZE + i])
                    regret_chunk_scalable.append(
                        self.buffer_chunk_scalable[self.buffer_chunk_num - REGRET_WINDOW_SIZE + i])
                    regret_chunk_remain_time.append(self.buffer_chunk_remain_time[
                                                        self.buffer_chunk_num - REGRET_WINDOW_SIZE + i] / MILLISECONDS_IN_SECOND)

            for i in range(REGRET_WINDOW_SIZE - len(regret_chunk_alternate_size)):
                regret_chunk_alternate_size.append([0, 0, 0, 0, 0, 0])
                regret_chunk_bitrate.append(0)
                regret_chunk_remain_time.append(0)
                regret_chunk_scalable.append(0)

            return_buffer_size = self.buffer_size

            video_chunk_remain = TOTAL_VIDEO_CHUNCK - self.video_chunk_counter
            next_video_chunk_sizes = []
            for i in range(QUALITY_DIM):
                if not self.download_complete:
                    if self.real_video_enabled:
                        next_video_chunk_sizes.append(self.video_size[self.video_chunk_counter][str(i)])
                    else:
                        next_video_chunk_sizes.append(self.video_size[i][self.video_chunk_counter])
                else:
                    next_video_chunk_sizes.append(0)
            return return_delay, \
                   0, \
                   return_buffer_size / MILLISECONDS_IN_SECOND, \
                   old_rebuff / MILLISECONDS_IN_SECOND, \
                   video_chunk_size, \
                   next_video_chunk_sizes, \
                   self.download_complete, \
                   0, \
                   video_chunk_remain, \
                   regret_chunk_remain_time, regret_chunk_bitrate, regret_chunk_alternate_size, \
                   video_chunk_counter_sent, \
                   regret_succeed, \
                   old_bitrate, \
                   last_bitrate, next_bitrate, latest_chunk_updated, self.buffer_chunk_num, regret_chunk_scalable
