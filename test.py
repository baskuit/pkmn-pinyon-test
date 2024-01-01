import numpy as np
import matplotlib.pyplot as plt


strategy_dict = {}

def get_dict ():
    # Open a text file in the current working directory
    file_path = "standard.txt"  # Replace with the actual file name
    with open(file_path, 'r') as file:
        # Iterate through each line in the file
        hp_1 = None
        hp_2 = None
        skip = False
        row_strategy = []
        col_strategy = []

        for line in file:
            # Process each line as needed
            # print(line.strip())  # Print or manipulate the line content as
            if line[0: 3] == "HP:":
                words = line.split(' ')
                # print(words)
                hp_1 = int(words[1])
                hp_2 = int(words[2])
                skip = False
                # print(words[3:])
                if (words[5] == '1' or words[6] == '0'):
                    skip = True
                for w in words[7:]:
                    if w[0] == '1':
                        skip = True
                
                # if not skip:
                    # print(line)

            if skip:
                continue

            if line[0:3] == "P1:":
                words = line.split(' ')
                row_strategy = []
                for w in words:
                    if w[-1] == ',':
                        x = float(w[:-1])
                        row_strategy.append(x)
            if line[0:3] == "P2:":
                words = line.split(' ')
                col_strategy = []
                for w in words:
                    if w[-1] == ',':
                        x = float(w[:-1])
                        col_strategy.append(x)
                
                # if (hp_1 > 300 and hp_2 > 300) :
                    # print('!')
                strategy_dict[(hp_1, hp_2)] = [row_strategy, col_strategy]
                assert(hp_1 >= hp_2)
                assert(len(row_strategy) == 3)
                assert(len(col_strategy) == 3)
                assert(row_strategy[2] == 0)


def color_function(a, b):

    x = int(a) + 1
    y = int(b) + 1
    z = None
    if x < y:
        z = strategy_dict[(y, x)][0]
    else:
        z = strategy_dict[(x, y)][0]
    # z = (a / 353.0 / 2, 0, b / 353.0 / 2)
    return z

get_dict()

image_array = np.zeros((353, 353, 3))
# print(image_array)

# Populate the arrays with the color values based on your function
for i in range(353):
    for j in range(353):
        r, g, b = color_function(i, j)
        image_array[(j, i, 0)] = r#int(r * 256)
        image_array[(j, i, 1)] = g#nt(g * 256)
        image_array[(j, i, 2)] = b#int(b * 256)

# Display the image
print(image_array[:, :, 2])
plt.imshow(image_array[:, :, 2])
plt.show()
