- Run two seperate server(s):
```
./im_server.exe
./im_server.exe 5001 6001 ../data
./im_server.exe 5002 6002 ../data
```
- Run Load Balancer
```
./load_balancer.exe
```

- Run clients
```
./im_client.exe
```

- Compile:

```
cd /s/Users/tanis/Desktop/NetworksProject/rewrite/src
g++ -std=gnu++17 -O2 im_server.cpp -lws2_32 -o im_server.exe
g++ -std=gnu++17 -O2 im_client.cpp -lws2_32 -o im_client.exe
g++ -std=gnu++17 -O2 load_balancer.cpp -lws2_32 -o load_balancer.exe

```


