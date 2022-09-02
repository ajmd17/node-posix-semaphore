{
    "targets": [
        {
            "target_name": "posix-semaphore",
            "sources": [
                "src/posix-semaphore.cpp"
            ],
            "include_dirs": [
                "<!(node -e \"require('nan')\")"
            ]
        }
    ]
}