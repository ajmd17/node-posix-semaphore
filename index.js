const sp = require('./build/Release/posix-semaphore.node');

/** @type {Semaphore[]} */
let openedSemaphores = [];

class Semaphore {
    /** @type {string} */
    name = null;

    /** @type {Buffer | null} */
    buffer = null;

    static Flag = {
        O_RDONLY: sp.O_RDONLY,
        O_RDWR: sp.O_RDWR,
        O_CREAT: sp.O_CREAT,
        O_EXCL: sp.O_EXCL,
        O_TRUNC: sp.O_TRUNC
    };

    static Mode = {
        S_IRUSR: sp.S_IRUSR,
        S_IWUSR: sp.S_IWUSR,
        S_IRGRP: sp.S_IRGRP,
        S_IWGRP: sp.S_IWGRP,
        S_IROTH: sp.S_IROTH,
        S_IWOTH: sp.S_IWOTH
    };

    /** @param {string} name */
    constructor(name) {
        this.name = name;
    }

    get isOpened() {
        return this.buffer != null;
    }

    /**
     * @param {Semaphore.Flag | number} oflag,
     * @param {Semaphore.Mode | number} mode,
     * @param {number} value
    */
    open(oflag, mode, value) {
        if (this.isOpened) {
            this.close();
        }

        openedSemaphores.push(this);
        this.buffer = sp._sem_open(this.name, oflag, mode, value);
    }

    wait() {
        if (!this.isOpened) {
            throw Error("Semaphore is not opened!");
        }

        sp._sem_wait(this.buffer);
    }

    post() {
        if (!this.isOpened) {
            throw Error("Semaphore is not opened!");
        }

        sp._sem_post(this.buffer);
    }

    close() {
        if (!this.isOpened) {
            return;
        }

        sp._sem_close(this.buffer);
        sp._sem_unlink(this.name);

        this.buffer = null;

        let index = openedSemaphores.indexOf(this);

        if (index != -1) {
            openedSemaphores.splice(index, 1);
        }
    }
    
    /**
     * @param {string} name
     * @param {Semaphore.Flag | number} oflag,
     * @param {Semaphore.Mode | number} mode,
     * @param {number} value
    */
    static open(name, oflag, mode, value) {
        let semaphore = new Semaphore(name);
        semaphore.open(oflag, mode, value);
        return semaphore;
    }
}

process.on("exit", () => {
    // release all semaphores
    // make a copy because close() modifies the list
    const currentlyOpenedSemaphores = [...openedSemaphores];

    for(let item of currentlyOpenedSemaphores) {
        if (item) {
            try {
                item.close();
            } catch (err) {
                console.error(`Failed to close named semaphore ${item.name}`, err);
            }
        }
    }
});

module.exports = Semaphore;
