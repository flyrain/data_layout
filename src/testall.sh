#test all
./signa -g ~/qemu/mem-2.6.26a1 ~/qemu/mem-2.6.26 > 2.6.26
echo './signa -g ~/qemu/mem-2.6.26a1 ~/qemu/mem-2.6.26 > 2.6.26'
./signa -g ~/qemu/mem-2.6.27 ~/qemu/mem-2.6.27-2 > 2.6.27
echo './signa -g ~/qemu/mem-2.6.27 ~/qemu/mem-2.6.27-2 > 2.6.27'
./signa -g ~/qemu/mem-2.6.28a ~/qemu/mem-2.6.28b > 2.6.28
echo './signa -g ~/qemu/mem-2.6.28a ~/qemu/mem-2.6.28b > 2.6.28'
./signa -g ~/qemu/mem-2.6.29a ~/qemu/mem-2.6.29a9 > 2.6.29
echo './signa -g ~/qemu/mem-2.6.29a ~/qemu/mem-2.6.29a9 > 2.6.29'
./signa -g ~/qemu/mem-2.6.31-10 ~/qemu/mem-2.6.31-20 > 2.6.31
echo './signa -g ~/qemu/mem-2.6.31-10 ~/qemu/mem-2.6.31-20 > 2.6.31'
./signa -g ~/qemu/mem-2.6.36.3-10 ~/qemu/mem-2.6.36.3-11 > 2.6.36.3
echo './signa -g ~/qemu/mem-2.6.36.3-10 ~/qemu/mem-2.6.36.3-11 > 2.6.36.3'
./signa -g ~/qemu/mem-2.6.36.4a3 ~/qemu/mem-2.6.36.4a > 2.6.36.4
echo '2.6.36.4'
./signa -g ~/qemu/mem-2.6.38.2-b ~/qemu/mem-2.6.38.2-a > 2.6.38.2
echo '2.6.38.2'
./signa -g ~/qemu/mem-3.0.0-11 ~/qemu/mem-3.0.0-20 > 3.0.0
echo '3.0.0'

#test bsd and windows
echo "./signa -g ~/qemu/mem-openbsd5-30 ~/qemu/mem-openbsd5-20 > openbsd5 "
./signa -g ~/qemu/mem-openbsd5-30 ~/qemu/mem-openbsd5-20 > openbsd5 
echo "./signa -g ~/qemu/mem-freebsd7.4-a ~/qemu/mem-freebsd7.4-c > freebsd7.4"
./signa -g ~/qemu/mem-freebsd7.4-a ~/qemu/mem-freebsd7.4-c > freebsd7.4
echo "./signa -g ~/qemu/mem-freebsd8.2-30  ~/qemu/mem-freebsd8.2-20  > freebsd8.2"
./signa -g ~/qemu/mem-freebsd8.2-30  ~/qemu/mem-freebsd8.2-20  > freebsd8.2
echo "./signa -g ~/qemu/mem-win7-a12 ~/qemu/mem-win7-a4 > win7"
./signa -g ~/qemu/mem-win7-a12 ~/qemu/mem-win7-a4 > win7
echo "./signa -g ~/qemu/mem-netbsd5.1.1b ~/qemu/mem-netbsd5.1.1a > netbsd5.1.1"
./signa -g ~/qemu/mem-netbsd5.1.1b ~/qemu/mem-netbsd5.1.1a > netbsd5.1.1
echo "./signa -g ~/qemu/mem-winvista-1 ~/qemu/mem-winvista-31 > winvista"
./signa -g ~/qemu/mem-winvista-1 ~/qemu/mem-winvista-31 > winvista
echo "./signa -g ~/qemu/mem-winxpsp2-a1 ~/qemu/mem-winxpsp2-a > winxpsp2"
./signa -g ~/qemu/mem-winxpsp2-a1 ~/qemu/mem-winxpsp2-a > winxpsp2
echo "./signa -g ~/qemu/mem-winxpsp3-20  ~/qemu/mem-winxpsp3-30 > winxpsp3"
./signa -g ~/qemu/mem-winxpsp3-20  ~/qemu/mem-winxpsp3-30 > winxpsp3 
echo "./signa -g ~/qemu/mem-winxp  ~/qemu/mem-winxp-2 > winxp"
./signa -g ~/qemu/mem-winxp  ~/qemu/mem-winxp-2 > winxp
