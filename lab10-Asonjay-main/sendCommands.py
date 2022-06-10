import time
import numpy as np
import socket


def main():
    x,y, ipAddress, locations = readConfigFile()
    x = int (x)
    y = int(y)
    try:
        controllerPort = input ("what port do you want to use for controller? ")
    except ValueError:
        print ("did you enter a port number? ")
        quit()
    try:
        myIPAddr = ipAddress[int(controllerPort)]
    except KeyError:
        print ("can't find your port in the config file")
        quit()
    a = createArray(x,y)
    mySocket = createSocket(myIPAddr)

    file = openFile()
    for line in file:
       print ("line is ", line)
       tokens = list(line.split())
       processCommand(tokens, ipAddress, locations,
                      controllerPort, mySocket)
       time.sleep (1)
               
    
    
def createArray (x,y):
   
    x =  np.arange(1,x*y+1).reshape(x,y)
    return x

def sendCommand(mySocket,ipaddr, port, string2Send):


    UDP_IP = ipaddr
    UDP_PORT = port
    
    MESSAGE = string2Send
    #print("UDP target IP: %s" % UDP_IP)
    #print("UDP target port: %s" % UDP_PORT)
    #print("message: ",  MESSAGE)


    mySocket.sendto(MESSAGE.encode(), (UDP_IP, UDP_PORT))

def printArray(a, x, y):
    for rows in range(x):
        for columns in range(y):
            print ("{0:10}".format(a[rows, columns]), end = "")
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
        
        
    #print (ipAddress) 
    return rows,columns, ipAddress, location

def getCommand():
    command = input ("what command would you like to send?\n" +
                     "if you want to send MSG, format is <port> <cmd> <SEQ#> <message>" + 
                     "Format <port> <cmd>, entering all 0's for port sends to everyone: ")
    return command

def getCommandFile(file):
    command = file.readline()
    return command

def openFile ():
    file = open ("commands.txt")
    return file

def processCommand(tokens, ipAddress, locations, myPort, mySocket):
    if tokens[1] == "MOV":
        port = int (tokens[0])
        if port == 0:
            for key in ipAddress:
                ipaddr = ipAddress[int(key)]
                string2Send = "6 " + "30 " + str(key) + " " + myPort + " 1 " + "MOV " + "1 " +   myPort + " " + tokens[2]
                #print (string2Send)
                sendCommand (mySocket, ipAddress[int(key)], key, string2Send);
        else:
            ipaddr = ipAddress[int(port)]
            #print ("Ip address for command is ", ipaddr)
            string2Send = "6 " + "30 " + tokens[0] + " " + myPort + " 1 " + "MOV " + "1 " +   myPort + " " + tokens[2]
            #print (string2Send)
            sendCommand (mySocket, ipaddr, port, string2Send);
                   
            sendCommand (mySocket, ipaddr, port, string2Send);
    elif tokens[1] == "LOC":
        port = int (tokens[0])
        if port == 0:
            for key in ipAddress:
                ipaddr = ipAddress[int(key)]

                string2Send = "6 " + "30 " + str(key) + " " + myPort + " 1 " + "LOC " + "1 " +   myPort +" " + " "
                #print (string2Send)
                sendCommand (mySocket, ipAddress[int(key)], key, string2Send);
        else:
            ipaddr = ipAddress[int(port)]
            #print ("Ip address for command is ", ipaddr)
            string2Send = "6 " + "30 " + tokens[0] + " " + myPort + " 1 " + "LOC " + "1 " +   myPort +" " + " "
            #print (string2Send)
            sendCommand (mySocket, ipaddr, port, string2Send);
    elif tokens[1] == "MSG":
        port = int (tokens[0])
        seqNumber = tokens [2]
        if port == 0:
            for key in ipAddress:
                ipaddr = ipAddress[int(key)]

                string2Send = "6 " + "30 " + str(port) + " " + myPort + " 4 " + "MSG " + seqNumber + " "+   myPort +" " + "HELLO-" + str(port) + "-" +seqNumber
                #print (string2Send)
                sendCommand (mySocket, ipAddress[int(key)], key, string2Send);
        else:
            for key in ipAddress:
                ipaddr = ipAddress[int(key)]
                string2Send = "6 " + "30 " + str(port) + " " + myPort +  " 4 " + "MSG " + seqNumber + " "+   myPort + " "  + "HELLO-" + str(port) + "-" + seqNumber
                sendCommand (mySocket, ipaddr, key, string2Send);



def getDataFromNetwork(mySocket):
    
    while True:
       data, address = mySocket.recvfrom(1024) # buffer size is 1024 bytes
      
       #print("received message: %s" % data)
       #str1 = data.decode('ascii')
       str1 = str(data)
       str1 = str1.replace ('b', ' ', 1)
       str1 = str1.replace ("'", " ", 2)
       fields = str1.split()
       #print ("the type of str1 is " , type(str1))
    #   print ("hello? " + str1)
    
def createSocket( myIPAddr):
       sock = socket.socket(socket.AF_INET, # Internet
                        socket.SOCK_DGRAM) # UDP
       
       return sock
 

main()
