import os



def load_trace(cooked_trace_folder):
    cooked_files = os.listdir(cooked_trace_folder)
    # import pdb; pdb.set_trace()
    all_cooked_time = []
    all_cooked_bw = []
    all_file_names = []
    for cooked_file in cooked_files:
        file_path = cooked_trace_folder + cooked_file
        cooked_time = []
        cooked_bw = []
        # print file_path
        with open(file_path, 'rb') as f:
            for line in f:
                parse = line.split()
                cooked_time.append(float(parse[0]))
                cooked_bw.append(float(parse[1]))
        all_cooked_time.append(cooked_time)
        all_cooked_bw.append(cooked_bw)
        # print("adding cook file:",cooked_file)
        all_file_names.append(cooked_file)

    return all_cooked_time, all_cooked_bw, all_file_names
