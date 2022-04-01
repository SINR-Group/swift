import configparser
import numpy as np

#First load and calculate all parameters from config file
CONF_FILE = "grad.ini"
conf = configparser.ConfigParser()
conf.read(CONF_FILE, encoding="utf-8")

#####################################################
GAMMA = conf.getfloat("net", "gamma")
QUALITY_DIM = conf.getint("base", "quality_levels")
ENTROPY_WEIGHT_START = conf.getfloat("net", "entropy_w_start")
ENTROPY_WEIGHT_END = conf.getfloat("net", "entropy_w_end")
DECAY_NUM = conf.getint("training", "decay_num")
ENTROPY_DECAY_ITERATIONS = conf.getint("net", "entropy_decay_iterations")
ITERATION_TIMES = ENTROPY_DECAY_ITERATIONS/(DECAY_NUM * 100)
LINEAR_DECAY_STEP = (ENTROPY_WEIGHT_START - ENTROPY_WEIGHT_END)/ITERATION_TIMES
ENTROPY_EPS = 1e-6
BUFFER_THRESH_SIZE = conf.getfloat("base", "buffer_size")
VIDEO_CHUNCK_SIZE = conf.getfloat("base", "video_chunk_size")
REGRET_WINDOW_SIZE = int(BUFFER_THRESH_SIZE/VIDEO_CHUNCK_SIZE) # the number of chunks to consider for qulity upgrade
if BUFFER_THRESH_SIZE%VIDEO_CHUNCK_SIZE!=0:
    REGRET_WINDOW_SIZE+=1
###The total number of actions
###Note that REGRET_WINDOW_SIZE-1 is to exclude the first chunk, which is being playedback
OUTPUT_DIM = QUALITY_DIM+ REGRET_WINDOW_SIZE-1+2+2
S_HISTORY = conf.getint("net", "history_window_size") # take how many action info in the past
FEATURE_DIM = conf.getint("net", "feature_dim")   # bit_rate, buffer_size, next_chunk_size, bandwidth_measurement(throughput and time), chunk_til_video_end,regret_chunk_scalable,regret_chunk_bit_rate,original_bitrate,target_bitrate
ACTOR_LR_RATE = conf.getfloat("net", "actor_learning_rate")
CRITIC_LR_RATE = conf.getfloat("net", "critic_learning_rate")
NUM_AGENTS = conf.getint("training", "agent_num")
TRAIN_SEQ_LEN = conf.getint("training", "train_seq_length")  # take as a train batch
MODEL_SAVE_INTERVAL = conf.getint("training", "model_save_interval")
VIDEO_BIT_RATE = eval(conf.get("base","video_bits"))  # Kbps
BUFFER_NORM_FACTOR = BUFFER_THRESH_SIZE
CHUNK_TIL_VIDEO_END_CAP = conf.getfloat("base", "chunk_num")
M_IN_K = 1000.0
REBUF_PENALTY = np.log2(VIDEO_BIT_RATE[-1]/float(VIDEO_BIT_RATE[0]))  # 1 sec rebuffering -> 3 Mbps
SMOOTH_PENALTY = 1
DEFAULT_QUALITY = 1  # default video quality without agent
RANDOM_SEED = 42
RAND_RANGE = 1000
T_NORM = 10.0
RESULTS_DIR = conf.get("log", "results_dir")
LOG_FILE = RESULTS_DIR + "/log"#conf.get("log", "log_file_path")
TEST_LOG_FOLDER = conf.get("log", "test_dir")
DEV_LOG_FOLDER = conf.get("log", "dev_dir")
TEST_LOG_FILE = TEST_LOG_FOLDER + "/log"
TRAIN_TRACES = conf.get("training", "train_traces_path")
DEV_TRACES = conf.get("training", "dev_traces_path")
TEST_TRACES = conf.get("testing", "test_traces_path")
NN_MODEL = conf.get("training", "existing_model")
if NN_MODEL == "none":
    NN_MODEL = None
PYTHON_EXECUTABLE_PATH = conf.get("base", "python_path")
REAL_CHUNK_SIZE_LOG = conf.get("base", "real_video_size_file")

#enum
TRAIN = 0
TEST = 1

####################################################

if __name__ == "__main__":
    print(type(NN_MODEL))
