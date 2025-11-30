FROM debian:bullseye

RUN apt-get update && apt-get install -y mariadb-server && rm -f /var/lib/apt/lists/*

COPY tools/init.sh /usr/local/bin/
COPY conf/50-server.cnf /etc/mysql/mariadb.conf.d

RUN chmod +x /usr/local/bin/init.sh

EXPOSE 3306

ENTRYPOINT ["/usr/local/bin/init.sh"]