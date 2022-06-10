import numpy as np
import socket


def main():
    x,y, ipAddress, locations = readConfigFile()
    x = int (x)
    y = int(y)
    try:
        myPort = input ("I am the controller\nwhat port do you want to use for controller? ")
    except ValueError:
        print ("did you enter a port number? ")
        quit()
    try:
        myIPAddr = ipAddress[int(myPort)]
    except KeyError:
        print ("can't find your port in the config file")
        quit()
    a = createArray(x,y)
    printArray(a, x, y)

    mySocket = createSocket(myPort, myIPAddr)

   # print(*[str(row)[1:-1]for row in a], sep='\n')



    getDataFromNetwork (mySocket,a, x, y, myPort)
       

def createArray(x,y):
    a = []
    for i in range (1, x*y+1):
        a.append(str(i))
    return a

        
def createArray2 (x,y):
   
    x =  np.arange(1,x*y+1).reshape(x,y)
    return x


def sendCommand(mySocket,ipaddr, port, string2Send):


    UDP_IP = ipaddr
    UDP_PORT = port
    
    MESSAGE = string2Send
    print("UDP target IP: %s" % UDP_IP)
    print("UDP target port: %s" % UDP_PORT)
    print("message: ",  MESSAGE)


    mySocket.sendto(MESSAGE.encode(), (UDP_IP, UDP_PORT))

def printArray(a, x, y):
    i = 0;
    print("\n\n\n\n\n\n\n\n")
    for rows in range(x):
        for columns in range(y):
            print ("{0:30}".format(a[i]), end = "")
            i= i+1
        print ()

def readConfigFile():
    filename = "config.txt"
    listOfAddresses = []
    ipAddress = {}
    location = {}
    file = open (filename)
    
    line = file.readline();
    rows, columns = line.split()
    for line in file:
        ip, port, loc = line.split()
        ipAddress[int(port)] = ip
        location [int(port)] = location
        
        

    return rows,columns, ipAddress, location

def getCommand():
    command = input ("what command would you like to send?\n" +
                     "Format <port> <cmd>, entering all 0's for port sends to everyone: ")
    return command


def processRCVCommand(fields, a, x, y, myPort):
    flag = 0
    try: 
        if fields[3] == myPort:
            return 0
        if fields[5] == "LOC":
            newLoc = int(fields[1])
            indx  = findElement (a, fields[3], x, y)
            if indx > -1: #remove it from table so it can move
                removeElement(a, fields[3], indx)
            a[newLoc-1] = str(a[newLoc-1]) + " "  + str(fields [3])

            newLoc = fields[1]
            fromPort = fields[3]
            return 1
        return 0
    except IndexError:
        print ("INDEX ERROR fields are ", fields)
        return 0



def getDataFromNetwork(mySocket, a, x, y, myPort):

    while True:
       data, address = mySocket.recvfrom(1024) # buffer size is 1024 bytes
      
       #print("received message: %s" % data)
       #str1 = data.decode('ascii')
       str1 = str(data)
       str1 = str1.replace ('b', ' ', 1)
       str1 = str1.replace ("'", " ", 2)
       fields = str1.split()
       flag = processRCVCommand(fields, a, x, y, myPort)
       if flag > 0:
           printArray(a, x, y)
    
def createSocket(myPort, myIPAddr):
       sock = socket.socket(socket.AF_INET, # Internet
                        socket.SOCK_DGRAM) # UDP
       sock.bind((myIPAddr, int(myPort)))
       return sock
 
def removeElement(a, element, indx):
    a[indx] = a[indx].replace(element, "")
    a[indx] = a[indx].replace("  ", " ")

    
def findElement (a, element,x, y):
    i = 0

    for rows in range(x):
        for columns in range(y):
            try:
                indx = a[i].index(element)
                
            except ValueError:
                indx = -1
            if indx > 0:
                return i
            i = i+1

    return -1
main()
