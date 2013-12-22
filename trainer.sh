network0=""
network1=${network0}"conv8x8:convs=16;snorm;"
network2=${network1}"conv8x8:convs=16;snorm;"
network3=${network2}"conv8x8:convs=16;snorm;"

model0="--model forward-network"
model1="--model forward-network --model-params ${network1}"
model2="--model forward-network --model-params ${network2}"
model3="--model forward-network --model-params ${network3}"

trainer="--trainer batch --trainer-params opt=lbfgs,iters=128,eps=1e-5"
#trainer="--trainer stochastic --trainer-params opt=asgd,batch=2"

params=""
params=${params}" --task mnist --task-dir /home/cosmin/experiments/databases/mnist/"
params=${params}" --loss logistic --trials 10 --threads 1"
params=${params}" ${trainer} ${model1}"

./build/ncv_trainer ${params}
