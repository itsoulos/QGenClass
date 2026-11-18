Fix QGenClass main.cpp: accept active_equal and targeted_equal as values for pop_mutationmethod.
Install from QGenClass root:
  cp main.cpp main.cpp.bak
  cp /path/to/extracted/main.cpp ./main.cpp
  make -j$(nproc)
Check ./QGenClass --help and test one short run for each mutation method.
