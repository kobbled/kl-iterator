TP_GROUPMASK = "1,*,*,*,*"

str  := SR[1]
reg1 := R[2]
reg2 := R[3]
poseset := PR[3]
poseget := PR[4]
check := F[20]


# PATH DATA TYPE
# ------------

.require :< "iteratorpath.tpp"

# reset path
tstist(Iterpath::RESET)

# build list
poseset.group(1) = Pos::setxyzgp(0,80,0,0,0,0)
tstist(Iterpath::PUSH, 'hello', 1, 3.14, &poseset)
poseset.group(1) = Pos::setxyzgp(48,64,10,0,0,0)
tstist(Iterpath::PUSH, 'world', 23, 6.28, &poseset)
poseset.group(1) = Pos::setxyzgp(77,21,20,0,0,0)
tstist(Iterpath::PUSH, 'foo', 70, 2.718, &poseset)
poseset.group(1) = Pos::setxyzgp(74,31,30,0,0,0)
tstist(Iterpath::PUSH, 'bar', 150, 0.707, &poseset)
poseset.group(1) = Pos::setxyzgp(39,-70,40,0,0,0)
tstist(Iterpath::PUSH, 'baz', -1, 0.5, &poseset)

check = tstist(Iterpath::ISNULL)
while (!check)
  tstist(Iterpath::NEXT)
  check = tstist(Iterpath::ISNULL)
  pause
end

# ------------

# ARRAY DATA TYPE
# ------------

.require :< "iteratorarray.tpp"

# reset path
tstias(Iterarr::RESET)

# build list
poseset.group(1) = Pos::setxyzgp(0,80,0,0,0,0)
tstias(Iterarr::PUSH, 'hello', 1, 3.14, &poseset)
poseset.group(1) = Pos::setxyzgp(48,64,10,0,0,0)
tstias(Iterarr::PUSH, 'world', 23, 6.28, &poseset)
poseset.group(1) = Pos::setxyzgp(77,21,20,0,0,0)
tstias(Iterarr::PUSH, 'foo', 70, 2.718, &poseset)
poseset.group(1) = Pos::setxyzgp(74,31,30,0,0,0)
tstias(Iterarr::PUSH, 'bar', 150, 0.707, &poseset)
poseset.group(1) = Pos::setxyzgp(39,-70,40,0,0,0)
tstias(Iterarr::PUSH, 'baz', -1, 0.5, &poseset)

check = tstias(Iterarr::ISNULL)
while (!check)
  tstias(Iterarr::NEXT)
  check = tstias(Iterarr::ISNULL)
  pause
end

