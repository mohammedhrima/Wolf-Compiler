cc main.c -lm -fsanitize=address -fsanitize=null -g3 
./a.out $1
rm -rf a.out.dSYM
rm -rf a.out 