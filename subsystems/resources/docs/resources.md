# Asynchronously Loading Resources

The resources subsystem is necessary to perform I/O operations, like loading files or web resources, in the background
without blocking worker threads and wasting computational resources.

Normally, a job would perform blocking I/O operations on its current worker thread and hinder other jobs to execute in
the meantime. 