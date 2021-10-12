import math

def get_euclidian_dist(vector_1, vector_2):
  sum = 0
  for qi, pi in zip(vector_1, vector_2):
      sum += pow(qi - pi, 2)
      
  return math.sqrt(sum)



node_snrs = [
[100,100,100,100],
[100,100,100,100],
[100,100,100,100],
[100,100,100,100],
[100,100,100,100],
[100,100,100,100],
[100,100,100,100],
[100,100,100,100],
]

f = open("trace-1-2-2-1-3-3-1",'r')

previous_mac_ap = ""
cont = 0

while True:
    line_0 = f.readline()
    if line_0 == '':
        break
    infos = line_0.split("\t")
    
    if previous_mac_ap == "":
      previous_mac_ap = infos[1]

    if previous_mac_ap != infos[1]:
      cont = cont + 1
      previous_mac_ap = infos[1]
    
    ap_mac_index = infos[1].split(":")

    ap_mac_index = int(ap_mac_index[5])-1
    
    sta_mac_index = infos[2].split(":")
    
    sta_mac_index = int(sta_mac_index[5])-1
    
    snr = infos[3].split("\n")
    
    snr = float(snr[0])
   
    node_snrs[sta_mac_index][ap_mac_index] = snr

groups = [[],[],[],[]]

for index_node, node_snr in enumerate(node_snrs):
    group_number = 0
    minimum_dist = float('inf')
    for index_ap in range(cont):
        euclidian_dist =  get_euclidian_dist(node_snrs[index_ap], node_snr)
        if ( euclidian_dist < minimum_dist):
            group_number = index_ap
            minimum_dist = euclidian_dist
    groups[group_number].append(index_node)
    
print(groups)