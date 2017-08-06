FROM alpine:3.6
RUN apk update && apk add nasm gcc musl-dev make
# RUN apk add libc-dev
WORKDIR /exp
COPY . .
