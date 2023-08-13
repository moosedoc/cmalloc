# cmalloc
Custom C Memory Allocator

This library is designed for embedded projects who cannot access the heap directly. Be it due to coding standards, regulations, etc.

This library allocates memory at compile time to the stack and utilizes a few interators to keep track of memory that has been allocated.

POOL_SZ is needed to be defined to determine the size of stack memory to utilize.

Define __64BIT if the project is being compiled & utilized in a 64 bit environment.

For standard allocator behavior, do not define _ENBL_DESCRIPTOR.

To enable more advanced allocator behaviors, define _ENBL_DESCRIPTOR. Defining _ENBL_DESCRIPTOR turns the library into a foundational file system allocator. This allows the library to increase previously allocated memories without affecting other allocated memory addresses. For example, if an object is allocated 30K, then a second object is allocated 10K. If the first object memory allocation is then increased by 10K, totaling 40K, the library handles the memory pointers such that the second object data is still accessible without having to do any pointer math project-side. In this environment, objects are defined to be double pointers. The top-level pointer always points to the address in the descriptor table. The descriptor table then points to the address of the memory pool. As objects are allocated, increased or freed, the descriptor address does not change but the address that it points to within the memory pool does change.
