# A Zero-Copy Shared Memory Ring Buffer
Build an inter-process communication (IPC) mechanism that allows two separate processes to pass high-throughput message data with sub-microsecond latency.

**The Architecture**: Use standard shared memory APIs (shm_open and mmap on Linux) to map a single region of memory into the virtual address space of both a producer process and a consumer process. Implement a circular ring buffer inside this space.

**Memory Management Focus**: Placement new to construct objects directly inside the shared memory segment, managing raw offsets rather than raw pointers (since the shared memory might be mapped to different virtual addresses in each process), and achieving true zero-copy serialization/deserialization.

**Concurrency Focus**: Coordinating the producer and consumer without OS-level mutexes wherever possible. You will rely on atomic read/write indices and memory barriers to ensure the consumer never reads an incomplete message and the producer never overwrites unread data.

## Implementation Plan:

**Phase 1**: OS Plumbing & Shared Mapping (3–5 Hours)
The initial hurdle is just getting two distinct processes to look at the exact same physical memory block safely.

- Tasks: Setting up shm_open and mmap. Writing a robust cleanup coordinator so that if one process crashes, the shared memory segment isn't orphaned or left in a corrupted state.
- The Trap: Realizing that pointers created by Process A are completely invalid in Process B because their virtual memory spaces differ. You have to pivot to tracking relative offsets from the base address of the shared mapping rather than storing raw pointers.

**Phase 2**: Layout & Control Structures (5–8 Hours)
Designing the precise layout of the shared control block.

- Tasks: Laying out the header (atomic read index, atomic write index, capacity) and the contiguous data array directly inside the raw byte buffer using placement new.
- The Trap: False sharing. If your read index and write index sit on the same 64-byte CPU cache line, the core running the producer and the core running the consumer will constantly invalidate each other's caches, destroying performance. You will need alignas(hardware_destructive_interference_size) to keep them separated.

**Phase 3**: The Concurrency Engine (8–12 Hours)
This is where the bulk of the intellectual heavy lifting happens. To make it highly performant, you will want to avoid OS mutexes and use atomic operations.

- Tasks: Writing the lock-free wrap-around logic for the indices.
- The Trap: Memory visibility. If the producer updates the write index before the actual message data has cleared the CPU's store buffers and hit shared cache, the consumer will read garbage. You have to move away from sequential consistency (std::memory_order_seq_cst) and carefully map out memory_order_release on writes and memory_order_acquire on reads.

**Phase 4**: Zero-Copy Serialization & Validation (6–10 Hours)
Ensuring data is constructed directly in place without intermediate copying.

- Tasks: Designing the API so the producer reserves a chunk of space, gets a raw pointer to that space, constructs the object directly in the ring buffer, and then commits it. Writing stress tests with high-frequency telemetry to verify zero data corruption under heavy load.

Scope: Using fixed-size messages (e.g. a flat struct containing raw data arrays), hitting the lower band of this timeline is possible. If supporting variable-length messages or complex types, the memory management complexity doubles, as a secondary allocator is needed inside the shared arena to manage those variable blocks.
