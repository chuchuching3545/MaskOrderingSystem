# MaskOrderingSystem

## 1. Problem Description
Due to the coronavirus, people are now strongly recommended to wear a mask in public. To make the process of ordering masks go smoothly, I implement a simplified mask pre-order system.

The system is composed of read and write servers, both can access a file `preorderRecord` that records infomation of consumer's order. When a server gets a request from clients, it will response according to the content of the file. A read server can tell the client how many masks can be ordered. A write server can modify the file to record the orders.

## 2. Running the Sample Servers

You can use the provided command to produce execution files first.
```bash=
$ gcc server.c -D READ_SERVER -o read_server
$ gcc server.c -o write_server
```

**Run**
The server will be listening to {port}
```bash=
$ ./write_server {port}
$ ./read_server {port}
```

## 3. Testing your Servers on Client Side

```shell=
telnet {hostname} {port}
```
If your server is running on linux1.csie.ntu.edu.tw, and listening to port 3333, your command on the client side will be `telnet linux1.csie.ntu.edu.tw 3333`

## 4. About the Preorder Record File 

The servers should be able to access the file `preorderRecord`.

The file includes 20 customer order structures, each with a consumer id (range from 902001 to 902020), and the number of masks each id can order.
10 adult masks and 10 children masks can be ordered in total with an id. 

```cpp=
typedef struct {
    int id; //902001-902020
    int adultMask; //set to 10 by default
    int childrenMask; //set to 10 by default
} Order
```

## 5. Sample input and output

**Read Server**

Clients can check how many masks they can order. 
Once it has connected to a read server, the terminal will show the following:

```shell
Please enter the id (to check how many masks can be ordered):
```

If you type an id (for example, `902001`) on the client side, the server shall reply:


```shell
You can order 10 adult mask(s) and 10 children mask(s).
```
and close the connection from client.


But, if someone else is ordering using the same id, the server shall reply:
```shell
Locked.
```
and close the connection from client.


**Write Server**

A consumer can make preorders. It will first show how many masks a consumer can order just like a read server, then ask for masktype and number of mask to preorder.

Once it connect to a write server, the terminal will show the following:
```shell
Please enter the id (to check how many masks can be ordered):
```
If you type an id (for example, `902001`) on the client side, the server shall reply the numbers, following by a prompt on the nextline.

```shell
You can order 10 adult mask(s) and 10 children mask(s).
Please enter the mask type (adult or children) and number of mask you would like to order:
```
If you type a command (for example, `adult 2`), the server shall modify the `preorderRecord` file and reply:
```
Pre-order for 902001 successed, 2 adult mask(s) ordered.
```
and close the connection from client.

But, if someone else is ordering using the same id, the server shall reply:
```shell
Locked.
```
and close the connection from client.

If the request cannot be process by the servers for other reasons, the servers shall reply:
```shell
Operation failed.
```
and close the connection from client.

