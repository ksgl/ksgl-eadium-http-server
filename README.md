# Build Instructions

```
mkdir build
cd build
cmake ..
cmake --build .
```

# Stress Test

```
sudo apt install siege
siege -c 8 -r 2000 http://localhost/
```
 
`-c` means the number of concurrent users

`-r` is for the amount of requests per user

The default directory for data is /var/www/html and /etc/httpd.conf for config file.
