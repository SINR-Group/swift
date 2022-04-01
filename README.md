Adaptive Video Streaming with Layered Neural Codecs
==========================================================

[Mallesham Dasari](), [Kumara Kahatapitiya](), [Samir R. Das](), [Aruna Balasubramanian](), [Dimitris Samaras](). <br />
NSDI 2022 (Conference on Networked Systems Design and Implementation). <br />
[[Paper](assets/nsdi-2022-paper.pdf)][[Slides]()][[Video]()]

Overview
========

The repository contains an implementation of the above paper. It has three independent codebases: 1) Motion flow extracter, 2) Layered neural codec located in the codec folder, 3) Adaptive video streaming pipeline using the proposed layered neural codec, located in streamer folder.

Network Architecture
--------------------

<p align="center">
  <img src="assets/codec.png" />
</p>

Quick Start
-----------

Swift's layered coding is built on top of [Pytorch-VCII](https://github.com/chaoyuaw/pytorch-vcii), and hence the running instructions are mostly similar. To run the code run `train.sh 2` (the argument (`0`, `1`, or `2`) specifies the level of hierarchy).

### Prerequisites

- Python3
- Pytorch
- FFMPEG

### Training the codec

- We used 64x64 patches of images with video samples of 10 seconds each to train the model.
- Prepare the data using the instructions from the flows directory in the root folder.
- To train the codec end-to-end, run `train.sh` with all three options (`0`, `1`, or `2`) to have a 3-level heirarchical coding.
- To test the codec, 


Citation
--------
If you find this useful, please use the following citation.
<pre>
@inproceedings {278366,
title = {Swift: Adaptive Video Streaming with Layered Neural Codecs},
author={Dasari, Mallesham and Kahatapitiya, Kumara and Das, Samir R. and Balasubramanian, Aruna and Samaras, Dimitris},
booktitle = {19th USENIX Symposium on Networked Systems Design and Implementation (NSDI 22)},
year = {2022},
address = {Renton, WA},
url = {https://www.usenix.org/conference/nsdi22/presentation/dasari},
publisher = {USENIX Association},
month = apr,
}
</pre>

Acknowledgements
----------------

The layered codec is implemented largely on top of [Pytorch-VCII](https://github.com/chaoyuaw/pytorch-vcii) originally developed by Chao-Yuan Wu and Biao Zhang. The streaming implementation follows [Grad: Learning for Overhead-aware Adaptive Video Streaming with Scalable Video Coding](http://jhc.sjtu.edu.cn/~bjiang/papers/Liu_MM2020_Grad.pdf) [MM'20] paper. 
