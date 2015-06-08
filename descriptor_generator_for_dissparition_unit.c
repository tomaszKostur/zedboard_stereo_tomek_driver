#include <stdio.h>
#include <stdlib.h>


//struct automat for matching two mask

typedef unsigned int u_int;

typedef struct generator_out
{
    u_int my_addr;
    u_int next_desc_addr;
    u_int zero;
    u_int source_addr;
    u_int destiny_addr;
    u_int bytes_to_transfer;
} g_out;

typedef struct generator_config
{
    u_int mask_size;
    u_int match_wide;
    u_int picture_wide;
    u_int descriptor_start_address;
    u_int l_picture_start_address;
    u_int r_picture_start_address;
    u_int destination_start_address;
    u_int unit_start_address;
    u_int mask_position_array_start_address;
    u_int match_position_array_start_address;

} g_conf;

g_out desc_generator_for_matching_unit(g_conf *config, unsigned long iteration, char last_iteration)
{
//    static u_int ret_pixel_counter;
    u_int mask_size = config -> mask_size;
    u_int match_wide = config -> match_wide;

    g_out the_descriptor;

    the_descriptor.my_addr = config -> descriptor_start_address + iteration * 0x40;
// circle of descriptors addresses
    if ( last_iteration == 0 )
    {
        the_descriptor.next_desc_addr = config -> descriptor_start_address + (iteration + 1) * 0x40;
    }
    else
    {
        the_descriptor.next_desc_addr = config -> descriptor_start_address;
    }

    the_descriptor.zero = 0;

    // number of reqired descriptors to get one result pixel
    u_int one_pix_desc_num = mask_size * mask_size
        + mask_size * match_wide
        +3; // mask_position, match_position, result;

    // number of actual return working pixel
    u_int wrk_pxl_num = iteration / one_pix_desc_num;
    // number for actual descriptor for actual working pixel
    u_int wrk_desc_num = iteration % one_pix_desc_num;

    // thanks to this construction the function can generate anyone descriptor
    // independent of previous. all parameters are calculated directly from
    // "iteration" variable
    if(wrk_desc_num < mask_size * mask_size)
    {
        // filling  mask
        u_int row = wrk_desc_num / mask_size;

        the_descriptor.source_addr = config -> l_picture_start_address
            + (wrk_desc_num % mask_size)
            + row * (config -> picture_wide)
            + wrk_pxl_num;

        the_descriptor.destiny_addr = config -> unit_start_address + wrk_desc_num;

    }
    else if (wrk_desc_num <mask_size * mask_size + mask_size * match_wide)
    {
        // filling match space
        // match position starts moving when mask reaches last of match possibillity
        if ( (wrk_pxl_num % config -> picture_wide) < (match_wide - (mask_size -1) / 2) )
        {
            u_int row = (wrk_desc_num - (mask_size * mask_size)) / match_wide;

            the_descriptor.source_addr = config -> r_picture_start_address
                + ((wrk_desc_num - mask_size * mask_size)) % mask_size
                + row * (config -> picture_wide);
//                + wrk_pxl_num;
        }
        else
        {
            u_int row = (wrk_desc_num - (mask_size * mask_size)) / match_wide;

            the_descriptor.source_addr = config -> r_picture_start_address
                + ((wrk_desc_num - mask_size * mask_size)) % mask_size
                + row * (config -> picture_wide);
                + (wrk_desc_num - match_wide);
        }

//        u_int row = (wrk_desc_num - (mask_size * mask_size)) / match_wide;
//
//        the_descriptor.source_addr = config -> r_picture_start_address
//            + ((wrk_desc_num - mask_size * mask_size)) % mask_size
//            + row * (config -> picture_wide);
//            + wrk_pxl_num;// this causes after matching matrix_l witch
//            matrix_r matrix_r is shift left as well as matrix_l;
//            witchout matrix_r is constans

        the_descriptor.destiny_addr = config -> unit_start_address + wrk_desc_num;
    }
    else if (wrk_desc_num == mask_size * mask_size + mask_size * match_wide)
    {
        // descriptor of mask_position
        // this function requires addictional array of mask_positions
        //
        // mask_position is: "wrk_pxl_num % picture_wide"
        // return picture is: "picture_wide - (mask_size-1)/2"
        // last pixells (number: "picture_wide - (mask_size-1)/2) are random
        // becouse of jumping to another row of picture since: "(mask_size-1)/2
        // pixells

        the_descriptor.source_addr = config -> mask_position_array_start_address + wrk_pxl_num;
        the_descriptor.destiny_addr = config -> unit_start_address + wrk_desc_num;

    }
    else if (wrk_desc_num == mask_size * mask_size + mask_size * match_wide + 1)
    {
        // descriptor of match_position
        // this function requires addictional array of match_positions
        //

        the_descriptor.source_addr = config -> match_position_array_start_address + wrk_pxl_num;
        the_descriptor.destiny_addr = config -> unit_start_address + wrk_desc_num;
    }
    else if (wrk_desc_num == mask_size * mask_size + mask_size * match_wide + 2)
    {
        // descriptor of result
        the_descriptor.source_addr = config -> unit_start_address;
        the_descriptor.destiny_addr = config -> destination_start_address + wrk_pxl_num;
    }

    the_descriptor.bytes_to_transfer = 4;

    return the_descriptor;
}

int main (int argc, char* argv[])
{
    // this is test for descriptors made witch descriptor generator function
#define TEST_LEN 20
    g_out descriptors[TEST_LEN];
    g_conf config;
// config init
    config.mask_size = 3;
    config.match_wide = 10;
    config.picture_wide = 30;
    config.descriptor_start_address = 0x11000000;
    config.l_picture_start_address = 0x11100000;
    config.r_picture_start_address = 0x11200000;
    config.destination_start_address = 0x10000000;
    config.unit_start_address = 0x4aa00000;
    config.mask_position_array_start_address = 0x12100000;
    config.match_position_array_start_address = 0x12200000;

    int i;
    for (i=0; i<TEST_LEN; ++i)
    {
        descriptors[i] = desc_generator_for_matching_unit( &config, i, 0 );
    }
    while(1)
    {
        printf("pokaz deskryptor nr=\n");
        int command;
        scanf("%d",&command);
        if(command >= TEST_LEN) break;

        printf("my_addr           = %x\n",descriptors[command].my_addr);
        printf("next_desc_addr    = %x\n",descriptors[command].next_desc_addr);
        printf("zero              = %x\n",descriptors[command].zero);
        printf("source_addr       = %x\n",descriptors[command].source_addr);
        printf("destiny_addr      = %x\n",descriptors[command].destiny_addr);
        printf("bytes_to_transfer = %x\n",descriptors[command].bytes_to_transfer);


    }

    return 0;
#undef TEST_LEN
}

