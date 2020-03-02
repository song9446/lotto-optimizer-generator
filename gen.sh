mkdir -p optimal_lotto_number_sets
min=1
max=100
for i in `seq $min $max`
do
    echo "julia gen.jl 45 6 $i 3 5000 10 > optimal_lotto_number_sets/$i.txt"
    julia gen.jl 45 6 $i 3 5000 10 > optimal_lotto_number_sets/$i.txt 
done
