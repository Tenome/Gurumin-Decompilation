// jpeg_api.h : the part of IJG libjpeg 6b that Packs.dll's image code calls (the library is linked statically).
// Struct layouts follow jpeglib.h 6b as built for Windows, where "boolean" is a single byte (the rpcndr.h type).
// Only the fields Packs.dll touches are used; the rest keep the library names so the sizes come out right
// (jpeg_error_mgr 0x84, jpeg_compress_struct 0x168, jpeg_decompress_struct 0x1b0).
#ifndef JPEG_API_H
#define JPEG_API_H

#include <stdio.h>

typedef unsigned char boolean;
typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int JDIMENSION;
typedef unsigned char JSAMPLE;
typedef JSAMPLE *JSAMPROW;
typedef JSAMPROW *JSAMPARRAY;

const int JPEG_LIB_VERSION = 62;      // libjpeg 6b
const int NUM_QUANT_TBLS = 4;
const int NUM_HUFF_TBLS = 4;
const int NUM_ARITH_TBLS = 16;
const int MAX_COMPS_IN_SCAN = 4;
const int C_MAX_BLOCKS_IN_MCU = 10;
const int D_MAX_BLOCKS_IN_MCU = 10;
const int JCS_RGB = 2;                // J_COLOR_SPACE value

struct jpeg_error_mgr {
    void (*error_exit)(void *cinfo);
    void (*emit_message)(void *cinfo, int msg_level);
    void (*output_message)(void *cinfo);
    void (*format_message)(void *cinfo, char *buffer);
    void (*reset_error_mgr)(void *cinfo);
    int msg_code;
    char msg_parm[80];                // union of ints and an 80 character string
    int trace_level;
    long num_warnings;
    const char * const *jpeg_message_table;
    int last_jpeg_message;
    const char * const *addon_message_table;
    int first_addon_message;
    int last_addon_message;
};

struct jpeg_compress_struct {
    jpeg_error_mgr *err;
    void *mem;
    void *progress;
    void *client_data;
    boolean is_decompressor;
    int global_state;
    void *dest;
    JDIMENSION image_width;           // pixels per row of the input
    JDIMENSION image_height;          // rows of the input
    int input_components;             // colour components per pixel (3 for RGB)
    int in_color_space;               // JCS_RGB
    double input_gamma;
    int data_precision;
    int num_components;
    int jpeg_color_space;
    void *comp_info;
    void *quant_tbl_ptrs[NUM_QUANT_TBLS];
    void *dc_huff_tbl_ptrs[NUM_HUFF_TBLS];
    void *ac_huff_tbl_ptrs[NUM_HUFF_TBLS];
    UINT8 arith_dc_L[NUM_ARITH_TBLS];
    UINT8 arith_dc_U[NUM_ARITH_TBLS];
    UINT8 arith_ac_K[NUM_ARITH_TBLS];
    int num_scans;
    const void *scan_info;
    boolean raw_data_in;
    boolean arith_code;
    boolean optimize_coding;
    boolean CCIR601_sampling;
    int smoothing_factor;
    int dct_method;
    unsigned int restart_interval;
    int restart_in_rows;
    boolean write_JFIF_header;
    UINT8 JFIF_major_version;
    UINT8 JFIF_minor_version;
    UINT8 density_unit;
    UINT16 X_density;
    UINT16 Y_density;
    boolean write_Adobe_marker;
    JDIMENSION next_scanline;         // next row jpeg_write_scanlines expects
    boolean progressive_mode;
    int max_h_samp_factor;
    int max_v_samp_factor;
    JDIMENSION total_iMCU_rows;
    int comps_in_scan;
    void *cur_comp_info[MAX_COMPS_IN_SCAN];
    JDIMENSION MCUs_per_row;
    JDIMENSION MCU_rows_in_scan;
    int blocks_in_MCU;
    int MCU_membership[C_MAX_BLOCKS_IN_MCU];
    int Ss, Se, Ah, Al;
    void *master;
    void *main;
    void *prep;
    void *coef;
    void *marker;
    void *cconvert;
    void *downsample;
    void *fdct;
    void *entropy;
    void *script_space;
    int script_space_size;
};

struct jpeg_decompress_struct {
    jpeg_error_mgr *err;
    void *mem;
    void *progress;
    void *client_data;
    boolean is_decompressor;
    int global_state;
    void *src;
    JDIMENSION image_width;
    JDIMENSION image_height;
    int num_components;
    int jpeg_color_space;
    int out_color_space;
    unsigned int scale_num, scale_denom;
    double output_gamma;
    boolean buffered_image;
    boolean raw_data_out;
    int dct_method;
    boolean do_fancy_upsampling;
    boolean do_block_smoothing;
    boolean quantize_colors;
    int dither_mode;
    boolean two_pass_quantize;
    int desired_number_of_colors;
    boolean enable_1pass_quant;
    boolean enable_external_quant;
    boolean enable_2pass_quant;
    JDIMENSION output_width;          // width after jpeg_start_decompress
    JDIMENSION output_height;
    int out_color_components;
    int output_components;
    int rec_outbuf_height;
    int actual_number_of_colors;
    JSAMPARRAY colormap;
    JDIMENSION output_scanline;       // next row jpeg_read_scanlines returns
    JDIMENSION input_scan_number;
    JDIMENSION input_iMCU_row;
    int output_scan_number;
    JDIMENSION output_iMCU_row;
    int (*coef_bits)[64];
    void *quant_tbl_ptrs[NUM_QUANT_TBLS];
    void *dc_huff_tbl_ptrs[NUM_HUFF_TBLS];
    void *ac_huff_tbl_ptrs[NUM_HUFF_TBLS];
    int data_precision;
    void *comp_info;
    boolean progressive_mode;
    boolean arith_code;
    UINT8 arith_dc_L[NUM_ARITH_TBLS];
    UINT8 arith_dc_U[NUM_ARITH_TBLS];
    UINT8 arith_ac_K[NUM_ARITH_TBLS];
    unsigned int restart_interval;
    boolean saw_JFIF_marker;
    UINT8 JFIF_major_version;
    UINT8 JFIF_minor_version;
    UINT8 density_unit;
    UINT16 X_density;
    UINT16 Y_density;
    boolean saw_Adobe_marker;
    UINT8 Adobe_transform;
    boolean CCIR601_sampling;
    void *marker_list;
    int max_h_samp_factor;
    int max_v_samp_factor;
    int min_DCT_scaled_size;
    JDIMENSION total_iMCU_rows;
    JSAMPLE *sample_range_limit;
    int comps_in_scan;
    void *cur_comp_info[MAX_COMPS_IN_SCAN];
    JDIMENSION MCUs_per_row;
    JDIMENSION MCU_rows_in_scan;
    int blocks_in_MCU;
    int MCU_membership[D_MAX_BLOCKS_IN_MCU];
    int Ss, Se, Ah, Al;
    int unread_marker;
    void *master;
    void *main;
    void *coef;
    void *post;
    void *inputctl;
    void *marker;
    void *entropy;
    void *idct;
    void *upsample;
    void *cconvert;
    void *cquantize;
    int unknown_tail;                 // 4 bytes the 0x1b0 struct size needs beyond the 6b fields listed above
};

extern "C" {
jpeg_error_mgr *jpeg_std_error(jpeg_error_mgr *err);
void jpeg_CreateCompress(jpeg_compress_struct *cinfo, int version, unsigned int structsize);
void jpeg_CreateDecompress(jpeg_decompress_struct *cinfo, int version, unsigned int structsize);
void jpeg_stdio_dest(jpeg_compress_struct *cinfo, FILE *outfile);
void jpeg_stdio_src(jpeg_decompress_struct *cinfo, FILE *infile);
void jpeg_set_defaults(jpeg_compress_struct *cinfo);
void jpeg_set_quality(jpeg_compress_struct *cinfo, int quality, boolean force_baseline);
void jpeg_simple_progression(jpeg_compress_struct *cinfo);
void jpeg_start_compress(jpeg_compress_struct *cinfo, boolean write_all_tables);
JDIMENSION jpeg_write_scanlines(jpeg_compress_struct *cinfo, JSAMPARRAY scanlines, JDIMENSION num_lines);
void jpeg_finish_compress(jpeg_compress_struct *cinfo);
void jpeg_destroy_compress(jpeg_compress_struct *cinfo);
int jpeg_read_header(jpeg_decompress_struct *cinfo, boolean require_image);
boolean jpeg_start_decompress(jpeg_decompress_struct *cinfo);
JDIMENSION jpeg_read_scanlines(jpeg_decompress_struct *cinfo, JSAMPARRAY scanlines, JDIMENSION max_lines);
boolean jpeg_finish_decompress(jpeg_decompress_struct *cinfo);
void jpeg_destroy_decompress(jpeg_decompress_struct *cinfo);
}

// jpeg_create_compress / jpeg_create_decompress are macros in jpeglib.h.
#define jpeg_create_compress(cinfo) jpeg_CreateCompress((cinfo), JPEG_LIB_VERSION, (unsigned int)sizeof(jpeg_compress_struct))
#define jpeg_create_decompress(cinfo) jpeg_CreateDecompress((cinfo), JPEG_LIB_VERSION, (unsigned int)sizeof(jpeg_decompress_struct))

#endif
