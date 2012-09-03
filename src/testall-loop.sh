#test all
./signa -g ~/qemu/mem-2.6.26a1 ~/qemu/mem-2.6.26 > 2.6.26-loop
echo './signa -g ~/qemu/mem-2.6.26a1 ~/qemu/mem-2.6.26 > 2.6.26'
./signa -g ~/qemu/mem-2.6.27 ~/qemu/mem-2.6.27-2 > 2.6.27-loop
echo './signa -g ~/qemu/mem-2.6.27 ~/qemu/mem-2.6.27-2 > 2.6.27'
./signa -g ~/qemu/mem-2.6.28a ~/qemu/mem-2.6.28b > 2.6.28-loop
echo './signa -g ~/qemu/mem-2.6.28a ~/qemu/mem-2.6.28b > 2.6.28'
./signa -g ~/qemu/mem-2.6.29a ~/qemu/mem-2.6.29a9 > 2.6.29-loop
echo './signa -g ~/qemu/mem-2.6.29a ~/qemu/mem-2.6.29a9 > 2.6.29'
./signa -g ~/qemu/mem-2.6.31-10 ~/qemu/mem-2.6.31-20 > 2.6.31-loop
echo './signa -g ~/qemu/mem-2.6.31-10 ~/qemu/mem-2.6.31-20 > 2.6.31'
./signa -g ~/qemu/mem-2.6.36.4a3 ~/qemu/mem-2.6.36.4a > 2.6.36.4-loop
echo '2.6.36.4'
./signa -g ~/qemu/mem-2.6.38.2-b ~/qemu/mem-2.6.38.2-a > 2.6.38.2-loop
echo '2.6.38.2'
./signa -g ~/qemu/mem-3.0.0-11 ~/qemu/mem-3.0.0-20 > 3.0.0-loop
echo '3.0.0'


#./signa -g ~/qemu/mem-2.6.26a1 ~/qemu/mem-2.6.26-10 ~/qemu/mem-2.6.26 >2.6.26
#./signa -g ~/qemu/mem-2.6.28a ~/qemu/mem-2.6.28b ~/qemu/mem-2.6.28x > 2.6.28
#./signa -g ~/qemu/mem-2.6.29a ~/qemu/mem-2.6.29a9 ~/qemu/mem-2.6.29a12 > 2.6.29 
