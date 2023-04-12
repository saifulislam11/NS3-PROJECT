from xml.etree import ElementTree as ET
import sys
import matplotlib.pyplot as pylab
from statistics import mean
#for csv
import csv 

et=ET.parse(sys.argv[1])
bitrates=[]
losses=[]
delays=[]
#drop
drop_ratio = []
delivery_ratio = []
totalrx = 0
totaltx = 0
packet_drop_count = 0
firstRx = 100000000000
lastRx = 0
totalBytesReceived = 0
for flow in et.findall("FlowStats/Flow"):
	for tpl in et.findall("Ipv4FlowClassifier/Flow"):
		if tpl.get('flowId')==flow.get('flowId'):
			break
	if tpl.get('destinationPort')=='654':
		continue
	losses.append(int(flow.get('lostPackets')))
	packet_drop_count+=int(flow.get('lostPackets'))
	
	rxPackets=int(flow.get('rxPackets'))
	totalrx+=int(flow.get('rxPackets'))
	totaltx+=int(flow.get('txPackets'))
	# for pktdrop in flow.findall('packetsDropped'):
	# 	packet_drop_count+=int(pktdrop.get('number'))
	# 	#print("drop count : \n",int(pktdrop.get('number')))
	if rxPackets==0:
		bitrates.append(0)
	else:
		#calculate throughput
		totalBytesReceived += int(flow.get('rxBytes'))
		if(float(flow.get('timeFirstRxPacket')[:-2])<firstRx):
			firstRx = float(flow.get('timeFirstRxPacket')[:-2])

		if(float(flow.get('timeLastRxPacket')[:-2])>lastRx):
			lastRx = float(flow.get('timeLastRxPacket')[:-2])
		t0=float(flow.get('timeFirstRxPacket')[:-2])
		t1=float(flow.get("timeLastRxPacket")[:-2])
		if (t1-t0)!=0:
			duration=(t1-t0)*1e-9
			bitrates.append(8*int(flow.get("rxBytes"))/duration*1e-3)
		
		delays.append(float(flow.get('delaySum')[:-2])*1e-9/rxPackets)

delivery_ratio.append(totalrx/totaltx)
drop_ratio.append(packet_drop_count/totaltx)

simTime = lastRx - firstRx
#print(simTime,"nanosecond", simTime*1e-9,"seconds")
througput = totalBytesReceived*8/(simTime*1e-6)
print("Throughput : %.2f" % (througput))
print("Average end-to-end delay : %.2f ms" % mean(delays))
#print("Average packet loss ratio : %.2f %%" % (mean(losses)))
print("Delivery ratio : %.2f" % (mean(delivery_ratio)))
print("Drop ratio : %.2f" % (mean(drop_ratio)))
pylab.subplot(311)
pylab.hist(bitrates,bins=40)
pylab.xlabel("Flow Bit Rates (b/s)")
pylab.ylabel("Number of Flows")


pylab.subplot(312)
pylab.hist(losses,bins=40)
pylab.xlabel("No of Lost Packets")
pylab.ylabel("Number of Flows")

pylab.subplot(313)
pylab.hist(delays,bins=10)
pylab.xlabel("Delay in Seconds")
pylab.ylabel("Number of Flows")

pylab.subplots_adjust(hspace=0.4)
pylab.savefig("results.pdf")

#csv insertion
with open("output.csv","a",newline="") as csv_file:
	writer = csv.writer(csv_file)
	writer.writerow([sys.argv[2],througput,mean(delays),mean(delivery_ratio),mean(drop_ratio)])
csv_file.close()
