What is guacd?
==============

[guacd](https://github.com/apache/guacamole-server/) is the native
server-side proxy used by the [Apache Guacamole web
application](http://guacamole.apache.org/).  If you wish to deploy
Guacamole, or an application using the [Guacamole core
APIs](http://guacamole.apache.org/api-documentation), you will need a
copy of guacd running.

How to use this image
=====================

Running guacd for use by the [Guacamole Docker image](https://registry.hub.docker.com/u/guacamole/guacamole/)
-----------------------------------------------------

    docker run --name some-guacd -d guacamole/guacd

guacd will be listening on port 4822, but this port will only be available to
Docker containers that have been explicitly linked to `some-guacd`.

Running guacd for use services by outside Docker
------------------------------------------------

    docker run --name some-guacd -d -p 4822:4822 guacamole/guacd

guacd will be listening on port 4822, and Docker will expose this port on the
same server hosting Docker. Other services, such as an instance of Tomcat
running outside of Docker, will be able to connect to guacd.

Beware of the security ramifications of doing this. There is no authentication
within guacd, so allowing access from untrusted applications is dangerous. If
you need to expose guacd, ensure that you only expose it as absolutely
necessary, and that only specific trusted applications have access. 

Connecting to guacd from an application
---------------------------------------

    docker run --name some-app --link some-guacd:guacd -d application-that-uses-guacd


Enabling guacd ssl
------------------
This explains how to enable ssl between guacamole and guacd using a self signed certificate.

1. Generate a new certificate
You need to create the new certificate on the guacd host.

    $ openssl genrsa -out /path/guacd/server.key 2048
    $ openssl req -new -key /path/guacd/server.key -out /path/guacd/cert.csr
    $ openssl x509 -in /path/guacd/cert.csr -out /path/guacd/server.crt -req -signkey /path/guacd/server.key -days 3650
    $ openssl pkcs12 -export -in /path/guacd/server.crt -inkey /path/guacd/server.key  -out /path/guacd/server.p12 -CAfile ca.crt -caname root

2. run guacd

    docker run --name some-guacd -d -p 4822:4822 \
      --env GUACD_CERTIFICATE_CRT=/etc/guacd/server.crt \
      --env GUACD_CERTIFICATE_KEY=/etc/guacd/server.key \
      --volume /path/guacd:/etc/guacd \
      guacamole/guacd

You may now enable, within Guacamole, guacd/proxy ssl connexion.

Reporting issues
================

Please report any bugs encountered by opening a new issue in
[our JIRA](https://issues.apache.org/jira/browse/GUACAMOLE/).

