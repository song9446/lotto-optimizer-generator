using LinearAlgebra
using Random
using StatsBase
using Combinatorics
import Base.print_matrix


function force(M, i)
    f = zeros(size(M, 1))
    for j in 1:K
        if i == j
            continue
        end
        #d = M[:, i] - M[:, j] .+ 0.00000000000001#+ (rand(N)* 2.0 .- 1.0)*0.00000001 # + rand(N)*0.025
        #d = M[:, i] - M[:, j] + rand(N)*0.025
        d = M[:, i] - M[:, j] + (rand(N) * 2.0 .- 1.0)*0.0125
        f += d * 1/(norm(d)^4.5)
    end
    return f
end

function run(N, C, R, K, ITER_NUM)
    # initial set of tickets(randomly assigned)
    M = zeros(N, K)
    for i in 1:K
        M[randperm(N)[1:C], i] .= 1
    end

    center = ones(N) * C/N

    #coverage = sum(M, dims=2)
    combination_indices = collect(combinations(1:C, R))
    coverage = zeros([N for i in 1:R]...)
    for col in eachcol(M)
        numbers = sort!(findall(!iszero, col))
        for i in combination_indices
            coverage[numbers[i]...] += 1.0
        end
    end

    #return [findall(!iszero, r) for r in eachcol(M)]
    for n in 1:ITER_NUM
        F = zeros(N, K)
        for i in 1:K
            F[:, i] = force(M, i)
        end
        f_mags = [norm(col) for col in eachcol(F)]
        #i = sample(Weights(f_mag))
        i = argmax(f_mags)
        d = normalize(M[:, i] + F[:, i] - center) + center - M[:, i]
        #d_max_index = sample(Weights(d))
        #d_min_index = sample(Weights(-d))
        d_max_index = argmax(d)
        d_min_index = argmin(d)
        if d_max_index != d_min_index && M[d_max_index, i] != 1.0
            M[d_max_index, i] = 1.0
            M[d_min_index, i] = 0.0
            new_f_mag = norm(force(M, i))
            if new_f_mag >= f_mags[i]
                #println(n, "th undo force: ", f_mags[i], "->", new_f_mag)
                M[d_max_index, i] = 0.0
                M[d_min_index, i] = 1.0
            end
        end
        #=
        if d_max_index != d_min_index && M[d_max_index, i] != 1.0
            last_nh = count(x -> x >= 1, coverage)
            last_numbers = sort!(findall(!iszero, vec(M[:, i])))
            for k in combination_indices
                coverage[last_numbers[k]...] -= 1.0
            end
            M[d_max_index, i] = 1.0
            M[d_min_index, i] = 0.0
            new_numbers = sort!(findall(!iszero, vec(M[:, i])))
            for k in combination_indices
                coverage[new_numbers[k]...] += 1.0
            end
            new_nh = count(x -> x >= 1, coverage)
            #println(n, "th hueristic: ", last_nh, "->", new_nh)
            if new_nh < last_nh
                #println("undo..", M[d_max_index, i], M[d_min_index, i])
                M[d_max_index, i] = 0.0
                M[d_min_index, i] = 1.0
                for k in combination_indices
                    coverage[new_numbers[k]...] -= 1.0
                    coverage[last_numbers[k]...] += 1.0
                end
            end
        end=#
    end
    return [findall(!iszero, r) for r in eachcol(M)]
end

N = parse(Int64, ARGS[1])
C = parse(Int64, ARGS[2])
K = parse(Int64, ARGS[3])
R = parse(Int64, ARGS[4])
ITER_NUM = parse(Int64, ARGS[5])
SEED = parse(Int64, ARGS[6])

#const N = 45
#const C = 6
#const R = 3
#const K = 50
#const SEED = 1
#const ITER_NUM = 1000
Random.seed!(SEED)

res = run(N, C, R, K, ITER_NUM)
for s in res
    for num in s
        print(num, ' ')
    end
    println()
end
