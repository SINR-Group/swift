import numpy as np
import tensorflow as tf
import tensorflow.compat.v1 as tf
tf.disable_v2_behavior()
import tflearn
from config import *


class ActorNetwork(object):
    """
    Input to the network is the state, output is the distribution
    of all actions.
    """
    def __init__(self, sess, state_dim, action_dim, learning_rate):
        self.sess = sess
        self.s_dim = state_dim
        self.a_dim = action_dim
        self.lr_rate = learning_rate
        self.count=0
        # Create the actor network
        self.inputs, self.out = self.create_actor_network()

        # Get all network parameters
        self.network_params = \
            tf.get_collection(tf.GraphKeys.TRAINABLE_VARIABLES, scope='actor')

        # Set all network parameters
        self.input_network_params = []
        for param in self.network_params:
            self.input_network_params.append(
                tf.placeholder(tf.float32, shape=param.get_shape()))
        self.set_network_params_op = []
        for idx, param in enumerate(self.input_network_params):
            self.set_network_params_op.append(self.network_params[idx].assign(param))

        # Selected action, 0-1 vector
        self.acts = tf.placeholder(tf.float32, [None, self.a_dim])
        self.entropy= tf.Variable(ENTROPY_WEIGHT_START,trainable=False,dtype=tf.float32)
        #self.decay_rate = tf.constant(LINEAR_DECAY_STEP,dtype=tf.float32)
        self.entropy_decrease = tf.assign(self.entropy,self.entropy-LINEAR_DECAY_STEP)
        self.act_grad_weights = tf.placeholder(tf.float32, [None, 1])

        # Compute the objective (log action_vector and entropy)
        self.obj = tf.maximum(tf.reduce_sum(tf.multiply(
                       tf.log(tf.reduce_sum(tf.multiply(self.out, self.acts),
                                            reduction_indices=1, keep_dims=True)+ENTROPY_EPS),
                       -self.act_grad_weights)) \
                   +  self.entropy*tf.reduce_sum(tf.multiply(self.out,
                                                           tf.log(self.out + ENTROPY_EPS))),-1000)
                     #+ tf.maximum(0.1,self.entropy)*tf.reduce_sum(tf.multiply(self.out,
                                                 #          tf.log(self.out + ENTROPY_EPS)))

        # Combine the gradients here
        self.actor_gradients = tf.gradients(self.obj, self.network_params)

        # Optimization Op
        self.optimize = tf.train.RMSPropOptimizer(self.lr_rate).\
            apply_gradients(zip(self.actor_gradients, self.network_params))

    def create_actor_network(self):
        with tf.variable_scope('actor'):
            inputs = tflearn.input_data(shape=[None, self.s_dim[0], self.s_dim[1]])

            split_0 = tflearn.fully_connected(inputs[:, 0:1, -1], 128, activation='relu')
            split_1 = tflearn.fully_connected(inputs[:, 1:2, -1], 128, activation='relu')
            split_2 = tflearn.conv_1d(inputs[:, 2:3, -S_HISTORY:], 128, 4, activation='relu')
            split_3 = tflearn.conv_1d(inputs[:, 3:4, -S_HISTORY:], 128, 4, activation='relu')
            #split_4 = tflearn.conv_1d(inputs[:, 4:5, :A_DIM], 256, 4, activation='relu')
            split_5 = tflearn.fully_connected(inputs[:, 5:6, -1], 128, activation='relu')
            split_6 = tflearn.conv_1d(inputs[:, 6:7, :REGRET_WINDOW_SIZE], 128, 4, activation='relu')
            split_7 = tflearn.conv_1d(inputs[:, 7:8, :REGRET_WINDOW_SIZE], 128, 4, activation='relu')
            #split_8 = tflearn.fully_connected(inputs[:, 8:9, -1], 128, activation='relu')
            #split_9 = tflearn.fully_connected(inputs[:, 9:10, -1], 128, activation='relu')

            split_2_flat = tflearn.flatten(split_2)
            split_3_flat = tflearn.flatten(split_3)
            #split_4_flat = tflearn.flatten(split_4)
            split_6_flat = tflearn.flatten(split_6)
            split_7_flat = tflearn.flatten(split_7)

            split_2_flat_deep = tflearn.fully_connected(split_2_flat, 128, activation='relu')
            split_3_flat_deep = tflearn.fully_connected(split_3_flat, 128, activation='relu')
            #split_4_flat_deep = tflearn.fully_connected(split_4_flat, 128, activation='relu')
            split_6_flat_deep = tflearn.fully_connected(split_6_flat, 128, activation='relu')
            split_7_flat_deep = tflearn.fully_connected(split_7_flat, 128, activation='relu')

            merge = [split_0, split_1, split_2_flat_deep, split_3_flat_deep, split_5,
                     split_6_flat_deep, split_7_flat_deep]#, split_8, split_9]
            # for i in range(REGRET_WINDOW_SIZE):
            #   merge.append(tflearn.flatten(tflearn.conv_1d(inputs[:, 10 + i:11 + i, :], 128, 4, activation='relu')))

            merge_net = tflearn.merge(
                merge, 'concat')

            dense_net_0 = tflearn.fully_connected(merge_net, 128, activation='relu')
            dense_net_1 = tflearn.fully_connected(dense_net_0, 64, activation='relu')
            out = tflearn.fully_connected(dense_net_1, self.a_dim, activation='softmax')

            return inputs, out

    def train(self, inputs, acts, act_grad_weights):

        self.sess.run(self.optimize, feed_dict={
            self.inputs: inputs,
            self.acts: acts,
            self.act_grad_weights: act_grad_weights
        })

    def predict(self, inputs):
        return self.sess.run(self.out, feed_dict={
            self.inputs: inputs
        })
    def decay_entropy(self):
        if self.sess.run(self.entropy)> 0.05:
            print("entropy:",self.sess.run(self.entropy_decrease))
        return #self.sess.run(self.step_increment)

    def get_gradients(self, inputs, acts, act_grad_weights):
        self.count+=1
        #print(self.count)
        return self.sess.run(self.actor_gradients, feed_dict={
            self.inputs: inputs,
            self.acts: acts,
            self.act_grad_weights: act_grad_weights
        })

    def apply_gradients(self, actor_gradients):
        return self.sess.run(self.optimize, feed_dict={
            i: d for i, d in zip(self.actor_gradients, actor_gradients)
        })

    def get_network_params(self):
        return self.sess.run(self.network_params)

    def set_network_params(self, input_network_params):
        self.sess.run(self.set_network_params_op, feed_dict={
            i: d for i, d in zip(self.input_network_params, input_network_params)
        })


class CriticNetwork(object):
    """
    Input to the network is the state and action, output is V(s).
    On policy: the action must be obtained from the output of the Actor network.
    """
    def __init__(self, sess, state_dim, learning_rate):
        self.sess = sess
        self.s_dim = state_dim
        self.lr_rate = learning_rate

        # Create the critic network
        self.inputs, self.out = self.create_critic_network()

        # Get all network parameters
        self.network_params = \
            tf.get_collection(tf.GraphKeys.TRAINABLE_VARIABLES, scope='critic')

        # Set all network parameters
        self.input_network_params = []
        for param in self.network_params:
            self.input_network_params.append(
                tf.placeholder(tf.float32, shape=param.get_shape()))
        self.set_network_params_op = []
        for idx, param in enumerate(self.input_network_params):
            self.set_network_params_op.append(self.network_params[idx].assign(param))

        # Network target V(s)
        self.td_target = tf.placeholder(tf.float32, [None, 1])

        # Temporal Difference, will also be weights for actor_gradients
        self.td = tf.subtract(self.td_target, self.out)

        # Mean square error
        self.loss = tflearn.mean_square(self.td_target, self.out)

        # Compute critic gradient
        self.critic_gradients = tf.gradients(self.loss, self.network_params)

        # Optimization Op
        self.optimize = tf.train.RMSPropOptimizer(self.lr_rate).\
            apply_gradients(zip(self.critic_gradients, self.network_params))

    def create_critic_network(self):
        with tf.variable_scope('critic'):
            inputs = tflearn.input_data(shape=[None, self.s_dim[0], self.s_dim[1]])

            split_0 = tflearn.fully_connected(inputs[:, 0:1, -1], 128, activation='relu')
            split_1 = tflearn.fully_connected(inputs[:, 1:2, -1], 128, activation='relu')
            split_2 = tflearn.conv_1d(inputs[:, 2:3, -S_HISTORY:], 128, 4, activation='relu')
            split_3 = tflearn.conv_1d(inputs[:, 3:4, -S_HISTORY:], 128, 4, activation='relu')
            #split_4 = tflearn.conv_1d(inputs[:, 4:5, :A_DIM], 256, 4, activation='relu')
            split_5 = tflearn.fully_connected(inputs[:, 5:6, -1], 128, activation='relu')
            split_6 = tflearn.conv_1d(inputs[:, 6:7, :REGRET_WINDOW_SIZE], 128, 4, activation='relu')
            split_7 = tflearn.conv_1d(inputs[:, 7:8, :REGRET_WINDOW_SIZE], 128, 4, activation='relu')
            #split_8 = tflearn.fully_connected(inputs[:, 8:9, -1], 128, activation='relu')
            #split_9 = tflearn.fully_connected(inputs[:, 9:10, -1], 128, activation='relu')

            split_2_flat = tflearn.flatten(split_2)
            split_3_flat = tflearn.flatten(split_3)
            #split_4_flat = tflearn.flatten(split_4)
            split_6_flat = tflearn.flatten(split_6)
            split_7_flat = tflearn.flatten(split_7)

            split_2_flat_deep = tflearn.fully_connected(split_2_flat, 128, activation='relu')
            split_3_flat_deep = tflearn.fully_connected(split_3_flat, 128, activation='relu')
            #split_4_flat_deep = tflearn.fully_connected(split_4_flat, 128, activation='relu')
            split_6_flat_deep = tflearn.fully_connected(split_6_flat, 128, activation='relu')
            split_7_flat_deep = tflearn.fully_connected(split_7_flat, 128, activation='relu')

            merge = [split_0, split_1, split_2_flat_deep, split_3_flat_deep, split_5,
                     split_6_flat_deep, split_7_flat_deep]#, split_8, split_9]
            # for i in range(REGRET_WINDOW_SIZE):
            #   merge.append(tflearn.flatten(tflearn.conv_1d(inputs[:, 10 + i:11 + i, :], 128, 4, activation='relu')))

            merge_net = tflearn.merge(
                merge, 'concat')

            dense_net_0 = tflearn.fully_connected(merge_net, 128, activation='relu')
            dense_net_1 = tflearn.fully_connected(dense_net_0, 64, activation='relu')
            out = tflearn.fully_connected(dense_net_1, 1, activation='linear')

            return inputs, out

    def train(self, inputs, td_target):
        return self.sess.run([self.loss, self.optimize], feed_dict={
            self.inputs: inputs,
            self.td_target: td_target
        })

    def predict(self, inputs):
        return self.sess.run(self.out, feed_dict={
            self.inputs: inputs
        })

    def get_td(self, inputs, td_target):
        return self.sess.run(self.td, feed_dict={
            self.inputs: inputs,
            self.td_target: td_target
        })

    def get_gradients(self, inputs, td_target):
        return self.sess.run(self.critic_gradients, feed_dict={
            self.inputs: inputs,
            self.td_target: td_target
        })

    def apply_gradients(self, critic_gradients):
        return self.sess.run(self.optimize, feed_dict={
            i: d for i, d in zip(self.critic_gradients, critic_gradients)
        })

    def get_network_params(self):
        return self.sess.run(self.network_params)

    def set_network_params(self, input_network_params):
        self.sess.run(self.set_network_params_op, feed_dict={
            i: d for i, d in zip(self.input_network_params, input_network_params)
        })


def compute_gradients(s_batch, a_batch, r_batch,r_batch_lable, terminal, actor,critic,last_fills):
    """
    batch of s, a, r is from samples in a sequence
    the format is in np.array([batch_size, s/a/r_dim])
    terminal is True when sequence ends as a terminal state
    """
    print("s_batch:",len(s_batch))
    print("r_batch:",len(r_batch))
    assert s_batch.shape[0] == a_batch.shape[0]
    assert s_batch.shape[0] == r_batch.shape[0]
    ba_size = s_batch.shape[0]

    v_batch = critic.predict(s_batch)

    R_batch = np.zeros(r_batch.shape)

    if terminal:
        R_batch[-1, 0] = 0  # terminal state
    else:
        R_batch[-1, 0] = v_batch[-1, 0]  # boot strap from last state

    #R_tmp = R_batch[-1, 0]
    #RE_tmp = 0

    for t in reversed(range(ba_size - 1)):
        '''
        if r_batch_lable[t] == 0:
            R_tmp = r_batch[t] + GAMMA * R_tmp
            # R_batch[t, 0] = r_batch[t] + GAMMA * R_tmp
            # R_tmp=R_batch[t,0]
        else:
            RE_tmp = r_batch[t] + GAMMA * RE_tmp
        R_batch[t, 0] = R_tmp + RE_tmp
        '''
        R_batch[t, 0] =r_batch[t]+GAMMA*v_batch[t+1,0]

    td_batch = R_batch - v_batch

    '''
    for t in reversed(range(ba_size)):
        # R_batch[t, 0] =r_batch[t]+GAMMA*v_batch[t+1,0]
        result = get_td_n(r_batch, v_batch, t, TD_STEPS)
        R_batch[t, 0] = result

    td_batch = R_batch - v_batch
    '''
    # print("r_batch:",R_batch)
    # print("v_batch:",v_batch)

    actor_gradients = actor.get_gradients(s_batch, a_batch, td_batch)
    critic_gradients = critic.get_gradients(s_batch, R_batch)

    return actor_gradients, critic_gradients, td_batch


def get_td_n(r_batch, v_batch, start_index, n_step):
    if start_index + n_step >= len(v_batch):
        result = 0
    else:
        result = v_batch[start_index + n_step, -1]

    for i in reversed(range(start_index, min(start_index + n_step, len(v_batch)))):
        result = r_batch[i] + GAMMA * result
    return result

def compute_entropy(x):
    """
    Given vector x, computes the entropy
    H(x) = - sum( p * log(p))
    """
    H = 0.0
    for i in range(len(x)):
        if 0 < x[i] < 1:
            H -= x[i] * np.log(x[i])
    return H

def compute_mask(current_buffer_num, current_chunk_bitrate):
    output = np.zeros([1, OUTPUT_DIM])

    start = 0
    if current_buffer_num > REGRET_WINDOW_SIZE:
        changable_length = REGRET_WINDOW_SIZE
    else:
        changable_length = current_buffer_num - 1

    for i in range(QUALITY_DIM):
        output[0][i] = 1
    output[0][-1] = 1
    output[0][-2] = 1
    output[0][-3] = 1
    output[0][-4] = 1
    for i in range(start, start + changable_length):
        if current_chunk_bitrate[i+1]<4:
            output[0][QUALITY_DIM+i] = 1

    return output
