# node-posix-semaphore

Experimental binding of POSIX sem_* functions for node.js
Do not use in production! There are a number of issues with this package, it was thrown together in a few hours pretty much.
A big glaring issue is that for simplicity, the `sem_t*` is stored in a JS buffer, and if that buffer's contents are modified from the JS side, you could have a default on your hands.
