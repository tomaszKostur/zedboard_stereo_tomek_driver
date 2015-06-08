#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>


void devmem_wr_0 (unsigned int physical_address, unsigned int data[] );

void devmem_multi_place_1_file_test();
void* get_mmap_pointer(int devmem_desc,size_t physical_address, size_t mem_size);

unsigned int make_dma_sg_descriptors_for_match_block(
        unsigned int match_size,
        unsigned int pixells_to_return,
        unsigned int descriptor_start_address,
        unsigned int left_matrix_start_address,
        unsigned int right_matrix_start_address,
        unsigned int destination_start_address);


int main(int argc, char *argv[])
{
    devmem_multi_place_1_file_test();
    return 0;
}

unsigned int make_dma_sg_descriptors_for_match_block(
        unsigned int match_size,
        unsigned int pixells_to_return,
        unsigned int descriptor_start_address,
        unsigned int left_matrix_start_address,
        unsigned int right_matrix_start_address,
        unsigned int destination_start_address)
{
#define DESCRIPTOR_CELLS 9
// deskryptor sklada sie z 8 pol, pierwsze dodatkowe bedzie aktualnym addresem
// wymaganym do zapisania w pamieci fizycznej

    unsigned long number_of_descriptors = match_size*match_size*
        pixells_to_return + pixells_to_return;

    unsigned int desc_array[number_of_descriptors][DESCRIPTOR_CELLS];

    unsigned long i;
    for(i=0; i<number_of_descriptors; ++i)
    {
        // descriptor demand physical address
        desc_array[i][0] = descriptor_start_adress + (i * 0x40);
        // next descriptor (physical) pointer
        desc_array[i][1] = descriptor_start_address + ( (i+1) *0x40);
        desc_array[i][2] = 0;// used in 64 adress space
        // source address (physical)
        desc_array[i][3] =
            // TODO do the state automata makes the switch between source
            // right and left pictures and destinations
    }


}

void devmem_multi_place_1_file_test()
{
// nie ma problemu zeby po otwarciu /dev/mem robic wskazniki do wielu
// miejsc w pamieci jednoczesnie
    int data1[9] = {1,2,3,4,5,6,7,8,9};
    int data2[9] = {9,8,9,8,9,8,9,8,9};
    // open /dev/mem as single file
    int devmem_desc = open ("/dev/mem", O_RDWR | O_SYNC );
    if(devmem_desc == -1)
    {
        perror("error opening /dev/mem");
        exit(EXIT_FAILURE);
    }
    // get two pointers
    void *pointer_to_mmap_data1, *pointer_to_mmap_data2;

    pointer_to_mmap_data1 = get_mmap_pointer( devmem_desc, 0x10000000,
            sizeof(data1) );

    pointer_to_mmap_data2 = get_mmap_pointer( devmem_desc, 0x11000000,
            sizeof(data1) );

    // write data to physical addresses
    int i;
    for( i=0; i < (sizeof( data1 ) / sizeof(int) ); ++i)
    {
        ((int *) pointer_to_mmap_data1)[i] = data1[i];
    }

    for( i=0; i< (sizeof( data2 ) / sizeof(int) ); ++i)
    {
        ((int *) pointer_to_mmap_data2)[i] = data2[i];
    }

    // unmap pointers
    int unmap_ret;
    unmap_ret = munmap( pointer_to_mmap_data1, sizeof(data1) );
    if(unmap_ret == -1)
    {
        perror("Error unmapping dp1");
        exit(EXIT_FAILURE);
    }

    unmap_ret = munmap( pointer_to_mmap_data1, sizeof(data1) );
    if(unmap_ret == -1)
    {
        perror("Error unmapping dp2");
        exit(EXIT_FAILURE);
    }

    close(devmem_desc);
    printf("test successfulu\n");
}

void* get_mmap_pointer(int devmem_desc,size_t physical_address, size_t mem_size)
{
    size_t alloc_mem_size, page_mask, page_size;
    void* mmap_pointer;

    page_size = sysconf( _SC_PAGESIZE );
    alloc_mem_size = (((mem_size / page_size) +1) * page_size);
    // wyrownanie mem_size do wielokrotnosci rozmiaru strony
    page_mask = ( page_size -1 );

    mmap_pointer = mmap( NULL, alloc_mem_size,
            PROT_READ | PROT_WRITE, MAP_SHARED,
            devmem_desc, ( physical_address & ~page_mask ) );
    // if error
    if( mmap_pointer == MAP_FAILED )
    {
        close( devmem_desc );
        perror("Error mmaping /dev/mem");
        exit(EXIT_FAILURE);
    }

    return mmap_pointer;
}

void close_devmem(void* mmap_pointer, size_t alloc_mem_size, int *devmem_desc)
{
    int munmap_ret = munmap(mmap_pointer, alloc_mem_size);
    if ( munmap_ret == -1)
    {
	perror("Error un-mmapping the /dev/mem");
        exit(EXIT_FAILURE);
    }

    int close_ret = close( *devmem_desc );
    if(close_ret == -1)
    {
        perror("Error closing /dev/mem");
    }

    printf("/dev/mem unmap and close successful\n");

}

void devmem_wr_0 (unsigned int physical_address, unsigned int data[] )
{
    size_t mem_size = sizeof(data);
    // Open a file for writing.
    int devmem_desc;

    devmem_desc = open("/dev/mem", O_RDWR | O_SYNC);
    if (devmem_desc == -1)
    {
	perror("Error opening /dev/mem for writing");
	exit(EXIT_FAILURE);
    }

    size_t alloc_mem_size, page_mask, page_size;
    void *mmap_pointer, *virt_addr;

    page_size = sysconf (_SC_PAGESIZE);
    alloc_mem_size = (((mem_size / page_size) + 1) * page_size);
    page_mask = (page_size -1);

    mmap_pointer = mmap(NULL, alloc_mem_size, PROT_READ | PROT_WRITE,
            MAP_SHARED,
            devmem_desc, (physical_address & ~page_mask));

    if (mmap_pointer == MAP_FAILED)
    {
	close(devmem_desc);
	perror("Error mmapping /dev/mem");
	exit(EXIT_FAILURE);
    }

    virt_addr = (mmap_pointer + (physical_address & page_mask));

    printf(" zaalokowano na adresie %p\n",mmap_pointer);

    /* Now write int's to the file as if it were memory (an array of ints).
     */
    int i;
    for(i=0; i<mem_size; ++i)
    {
        ((int*)mmap_pointer)[i] = data[i];
    }

//    *(unsigned int*)mmap_pointer = *data;

    /* Don't forget to free the mmmap_pointerped memory
     */
    if (munmap(mmap_pointer, alloc_mem_size) == -1) {
	perror("Error un-mmapping the file");
        exit(EXIT_FAILURE);
	/* Decide here whether to close(devmem_desc) and exit() or not. Depends... */
    }

    /* Un-mmaping doesn't close the file, so we still need to do that.
     */
    close(devmem_desc);
    printf("/dev/mem operations successful\n");

}
