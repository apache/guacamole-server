# Build the development image
docker build -f Dockerfile.dev -t guacamole-server-dev .

# Run the development container
docker run -it -v $(pwd):/home/guacamole-server guacamole-server-dev

# Inside the container, build guacamole-server:
autoreconf -fiv
./configure --prefix=/opt/guacamole --with-systemd-dir=/etc/systemd/system
make clean && make -j$(nproc)
make install