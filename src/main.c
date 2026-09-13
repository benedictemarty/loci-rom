#include <loci.h>
#include "keyboard.h"
#include "tui.h"
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "libsrc/dir.h"
#include "libsrc/dirent.h"
#include "persist.h"
#include "filemanager.h"
#include "snapshot.h"

extern uint8_t irq_ticks;
#pragma zpsym ("irq_ticks")

extern uint8_t irq_ticks;
#pragma zpsym ("irq_ticks")

char txt_title[40];
const char txt_menu[] = "Select";
const char txt_mdisc[] = "Microdisc";
const char txt_df0[] = "A:";
const char txt_df1[] = "B:";
const char txt_df2[] = "C:";
const char txt_df3[] = "D:";
const char txt_tape[] = "Cassette";
const char txt_tap[] = "tap:";
const char txt_auto[] = "\024Auto \x10";
const char txt_bit[] = "\024Bits \x10";
const char txt_cload[] = "\021CLOAD \x10";
const char txt_orom[] = "Oric ROM";
const char txt_rom[] = "rom:";
const char txt_basic11[] = "\024Atmos \x10";
const char txt_basic10[] = "\024Oric-1\x10";
const char txt_custom[] = "\021Custom\x10";
const char txt_mouse[] = "Mouse";
const char txt_x[] = "[x]";
const char txt_on[] = "\x14on  \x10";
const char txt_off[] = "\x11off \x10";
const char txt_std[] = "\010";
const char txt_alt[] = "\011";
const char txt_usb[] = "\x09\x04#%&()";
const char txt_rew[] = "*";
const char txt_neg[] = "-";
const char txt_ffw[] = "+";
const char txt_eject[] = ",";
const char txt_locked[] = "!";
const char txt_unlocked[] = "\"";
const char txt_warn_sign[] = "\x01!\x03";
const char txt_dir_warning[] = "Max files. Use filter";
const char txt_boot[] = "\x13\004Boot  ";
const char txt_return[] = "\x13\004Return  \x10";
const char txt_spinner[] = "/-\\|";
const char txt_map[] = "RV1 adjust";
const char txt_timing[] = "Timing";
const char txt_filter[] = "[      ]";
const char txt_booting[] = "Booting";
const char txt_returning[] = "Returning";
const char txt_help[] = "\006ESC\003= boot\006RETURN\003= return";
char txt_rv1[] = "--";
char txt_tior[] = "tior --";
char txt_tiow[] = "tiow --";
char txt_tiod[] = "tiod --";
char txt_tadr[] = "tadr --";
char txt_cnt[8];

char filter[6] = ".dsk";

uint8_t rv1;
uint8_t spin_cnt;

char* dbg_status = TUI_SCREEN_XY_CONST(35,1);
#define DBG_STATUS(fourc) strcpy(dbg_status,fourc)

char tmp_str[256];

#define DIR_BUF_SIZE 3072
char dir_buf[DIR_BUF_SIZE];
char** dir_ptr_list = (char **)&dir_buf[DIR_BUF_SIZE];  //Reverse array
unsigned int dir_entries;
int dir_offset;
char dir_lpage[2];
char dir_rpage[2];
uint8_t dir_needs_refresh;
#define DIR_PAGE_SIZE 24

bool return_possible;

struct _loci_cfg loci_cfg;

extern void init_display(void);

uint8_t auto_tune_tior(void);
int dir_cmp(const void *lhsp,const void *rhsp);
uint8_t dir_fill(char* dname);
uint8_t tap_fill(void);
void parse_files_to_widget(void);
uint8_t update_dir_ui(void);
void boot(bool do_return);
void update_onoff_btn(uint8_t idx, uint8_t on);
void update_mode_btn(void);
void update_rom_btn(void);
void update_eject_btn(uint8_t drv);
void update_load_btn(void);
void update_tap_counter(void);
void do_eject(uint8_t drv, uint8_t ui_idx);
void DisplayKey(unsigned char key);
unsigned char Mouse(unsigned char key);
uint8_t auto_tune_tior(void);
void map_step(int8_t d);
void main(void);

tui_widget ui[] = {
    { TUI_START, 1, 0, 0, 0 },
    //{ TUI_BOX,  39,28, 0, 0 },
    { TUI_TXT,   1, 0, 40, txt_title },
    { TUI_TXT,   1, 2,10, txt_mdisc }, { TUI_SEL, 12, 2, 6, txt_off },
    { TUI_TXT,   5, 3, 4, txt_df0 }, { TUI_SEL,   8, 3,26, loci_cfg.drv_names[0] },
    { TUI_TXT,   5, 4, 4, txt_df1 }, { TUI_SEL,   8, 4,26, loci_cfg.drv_names[1] },
    { TUI_TXT,   5, 5, 4, txt_df2 }, { TUI_SEL,   8, 5,26, loci_cfg.drv_names[2] },
    { TUI_TXT,   5, 6, 4, txt_df3 }, { TUI_SEL,   8, 6,26, loci_cfg.drv_names[3] },
    { TUI_TXT,   1, 8,10, txt_tape },{ TUI_SEL, 12, 8, 6, txt_off },
    { TUI_SEL,  19, 8, 7, txt_auto },{ TUI_SEL, 27, 8, 8, txt_cload },
    { TUI_TXT,   3, 9, 4, txt_tap }, { TUI_SEL,   8, 9,18, loci_cfg.drv_names[4] },
    { TUI_TXT,   1,11, 8, txt_orom }, { TUI_SEL,  12,11, 8, txt_basic11 },
    { TUI_TXT,   3,12, 4, txt_rom },  { TUI_SEL,   8,12,26, loci_cfg.drv_names[5] },
    { TUI_TXT,   1,15,10, txt_mouse }, { TUI_SEL, 12,15, 6, txt_off },
    { TUI_TXT,   1,19,10, txt_map },
    { TUI_SEL,  30, 19, 1, txt_neg},
    { TUI_TXT,  29, 19, 1, txt_alt},
    { TUI_TXT,  33, 19, 2, txt_rv1},
    { TUI_TXT,  29, 19, 1, txt_std},
    { TUI_SEL,  36, 19, 1, txt_ffw},

    { TUI_NOP,  32, 0, 7, txt_usb},

    { TUI_TXT,  26,  9, 1, txt_alt},
    { TUI_SEL,  27,  9, 1, txt_rew},
    { TUI_SEL,  28,  9, 7, txt_cnt},
    { TUI_SEL,  35,  9, 1, txt_ffw},
    { TUI_SEL,  36,  9, 1, txt_eject},
    { TUI_TXT,  35,  3, 1, txt_alt},
    { TUI_TXT,  35,  4, 1, txt_alt},
    { TUI_TXT,  35,  5, 1, txt_alt},
    { TUI_TXT,  35,  6, 1, txt_alt},
    { TUI_SEL,  36,  3, 1, txt_eject},
    { TUI_SEL,  36,  4, 1, txt_eject},
    { TUI_SEL,  36,  5, 1, txt_eject},
    { TUI_SEL,  36,  6, 1, txt_eject},
    { TUI_TXT,  35, 12, 1, txt_alt},
    { TUI_TXT,  36, 12, 1, txt_eject},
    { TUI_NOP,  38,  3, 1, txt_unlocked},
    { TUI_NOP,  38,  4, 1, txt_locked},
    { TUI_NOP,  38,  5, 1, txt_locked},
    { TUI_NOP,  38,  6, 1, txt_unlocked},
    { TUI_TXT,  37,  9, 1, txt_alt},
    { TUI_NOP,  38,  9, 1, txt_locked},
    { TUI_NOP,  20, 27,11, txt_return},
    { TUI_SEL,  31, 27, 8, txt_boot},

    { TUI_TXT,   1, 20, 8, txt_timing},
    { TUI_TXT,  30, 20, 8, txt_tior},
    { TUI_TXT,  30, 21, 8, txt_tiow},
    { TUI_TXT,  30, 22, 8, txt_tiod},
    { TUI_TXT,  30, 23, 8, txt_tadr},
    { TUI_TXT,   1, 26,32, txt_help},

    { TUI_END,   0, 0, 0, 0 }
};
#define IDX_FDC_ON 3
#define IDX_DF0 5
#define IDX_DF1 7
#define IDX_DF2 9
#define IDX_DF3 11
#define IDX_TAP_ON 13
#define IDX_TAP_BIT 14
#define IDX_TAP_LOAD 15
#define IDX_TAP 17
#define IDX_ROM 19
#define IDX_ROM_FILE 21
#define IDX_MOU_ON 23
#define IDX_MAP_REW 25
#define IDX_MAP_RV1 27
#define IDX_MAP_FFW 29
#define IDX_TAP_REW 32
#define IDX_TAP_CNT 33
#define IDX_TAP_FFW 34
#define IDX_EJECT_TAP 35
#define IDX_EJECT_DF0 40
#define IDX_EJECT_DF1 41
#define IDX_EJECT_DF2 42
#define IDX_EJECT_DF3 43
#define IDX_EJECT_ROM 45
#define IDX_RETURN 52
#define IDX_BOOT 53
#define IDX_TIOR 55

const uint8_t tui_eject_idx[] = { 
    IDX_EJECT_DF0, 
    IDX_EJECT_DF1,
    IDX_EJECT_DF2,
    IDX_EJECT_DF3,
    IDX_EJECT_TAP,
    IDX_EJECT_ROM,
};

#define POPUP_FILE_START 8
/* Seules les POPUP_FILE_START premières entrées sont fixes ; les autres sont
 * (re)remplies par parse_files_to_widget(). Le tableau vit en BSS (pas de queue
 * de zéros stockée en ROM) et le gabarit constant est copié au démarrage. */
tui_widget popup[POPUP_FILE_START+DIR_PAGE_SIZE+1];
static const tui_widget popup_tpl[POPUP_FILE_START+1] = {
    { TUI_START, 2, 2, 0, 0 },
    { TUI_BOX,  38,26, 0, 0 },
    { TUI_TXT,   1, 0,25, loci_cfg.path},
    { TUI_TXT,  26, 0, 8, txt_filter},
    { TUI_INP,  27, 0, 6, filter},
    { TUI_SEL,  34, 0, 3, txt_x},
    { TUI_TXT,  33,25, 1, dir_lpage},
    { TUI_TXT,  34,25, 1, dir_rpage},
    { TUI_END,   0, 0, 0, 0 }
};
#define IDX_PATH 2
#define IDX_FILTER 4
#define IDX_XPAGE 5
#define IDX_LPAGE 6
#define IDX_RPAGE 7

tui_widget warning[] = {
    { TUI_START, 4,10, 0, 0},
    { TUI_BOX,  32, 3, 0, 0},
    { TUI_INV,   1, 1,  3, txt_warn_sign},
    { TUI_TXT,   5, 1, 25, txt_dir_warning},
    { TUI_END,   0, 0, 0, 0}
};

// dir_cmp -- compare directory entries
int dir_cmp(const void *lhsp,const void *rhsp)
{
    const char *lhs = *((const char**)lhsp);
    const char *rhs = *((const char**)rhsp);
    int cmp;

    cmp = stricmp(lhs,rhs);

    //Sort dirs before files by inverting result
    if(lhs[0] != rhs[0]){
        return -cmp;
    }else{
        return cmp;
    }
}

/* Fill the directory buffer with filenames from the bottom
   and pointers from the top.
   Returns 0 if buffer becomes full before all dir entries are captured
*/
uint8_t dir_fill(char* dname){
    DIR* dir;
    struct dirent* fil;
    uint16_t tail;     //Filename buffer tail
    uint8_t ret;
    int len;

    if(!dir_needs_refresh){
    //    return 1;
    }
    //DBG_STATUS("odir");
    dir = opendir(dname);
    if(dname[0]==0x00){     //Root/device list
        tail = 0;
        dir_entries = 0;
    }else{                  //Non-root
        strcpy(dir_buf,"/..");
        dir_ptr_list[-1] = dir_buf;
        tail = 4; //strlen("/..")+1
        dir_entries = 1;
    }
    dir_offset = 0;
    ret = 1;
    //DBG_STATUS("rdir");
    while(tail < DIR_BUF_SIZE){             //Safeguard
        do {
            fil = readdir(dir);
        }while(fil->d_name[0]=='.');        //Skip hidden files
        if(fil->d_name[0] == 0){            //End of directory listing
            break;
        }
        dir_ptr_list[-(++dir_entries)] = &dir_buf[tail];  
        if(fil->d_attrib & DIR_ATTR_DIR){
            dir_buf[tail++] = '/';
        }else if(fil->d_attrib & DIR_ATTR_SYS){
            dir_buf[tail++] = '[';
        }else{
            if(filter[0]){
                if(!strcasestr(fil->d_name, filter)){
                    dir_entries--;  //roll-back
                    continue;       //next file
                }
            }
            dir_buf[tail++] = ' ';
        }
        len = strlen(fil->d_name);
        if(len > (DIR_BUF_SIZE-tail-(dir_entries*sizeof(char*)))){
            ret = 0;                     //Buffer is full
            dir_entries--;                //Rewind inclomplete entry
            break;
        }else{
            strcpy(&dir_buf[tail], fil->d_name);
            tail += len + 1;
        }
    }
    //DBG_STATUS("cdir");
    closedir(dir);
    //DBG_STATUS("    ");

    qsort(&dir_ptr_list[-(dir_entries)], dir_entries, sizeof(char*), dir_cmp);
    dir_needs_refresh = 0;
    return ret;
}

/* Reuse dir_buf for tape content listing */
uint8_t tap_fill(void){
    tap_header_t hdr;
    long counter;
    unsigned int start_addr, end_addr, size;
    dir_entries = 0;
    dir_offset = 0;
    
    TAP.cmd = TAP_CMD_REW;
                        
    //Using fixed 64 byte entries for tape content
    while((dir_entries < (DIR_BUF_SIZE/64))){
        counter = tap_read_header(&hdr);
        if(counter == -1)
            break;
        start_addr = (unsigned int)((hdr.start_addr_hi<<8) | hdr.start_addr_lo);
        end_addr =   (unsigned int)((hdr.end_addr_hi<<8)   | hdr.end_addr_lo);
        if(start_addr > end_addr)   //Bad/unsupported header
            size = 0;
        else
            size = end_addr - start_addr;
        *((long*)(&dir_buf[dir_entries*64])) = counter;
        sprintf(&dir_buf[(dir_entries*64) + 4]," %d %-12.12s %-3s $%04X %5db",
            dir_entries + 1,
            hdr.filename[0] ? (char*)hdr.filename : "<no name>",
            hdr.type == 0x80 ? "BIN" : "BAS",    //TODO Incomplete Type decode
            start_addr, 
            size
        );
        dir_ptr_list[-(dir_entries+1)] = &dir_buf[(dir_entries*64)+4];
        dir_entries++;
        //Seek over file on tape
        counter += sizeof(tap_header_t) + 4 + size;
        tap_seek(counter);
    }
    qsort(&dir_ptr_list[-(dir_entries)], dir_entries, sizeof(char*), dir_cmp);
    return dir_entries;
}

void parse_files_to_widget(void){
    uint8_t i;
    char** dir_idx;
    tui_widget* widget;

    //Directory page out-of-bounds checks
    if(dir_offset >= dir_entries){
        dir_offset -= DIR_PAGE_SIZE;
    }
    if(dir_offset < 0){
        dir_offset = 0;
    }
    dir_idx = &dir_ptr_list[-(dir_entries-dir_offset)]; //(char**)(dir_ptr_list - dir_entries + offset);
    widget = &popup[POPUP_FILE_START]; //(tui_widget*)(popup + POPUP_FILE_START);

    for(i=0; (i < DIR_PAGE_SIZE) && ((i+dir_offset) < dir_entries); i++){
        widget->type = TUI_SEL;
        widget->x = 1;
        widget->y = i+1;
        widget->len = 34;
        widget->data = dir_idx[i]; //dir_ptr_list[-(dir_entries-offset-i)];
        widget = &widget[1];
    }
    widget->type = TUI_END;

    dir_lpage[0] = '-';
    dir_rpage[0] = '-';
    popup[IDX_LPAGE].type = TUI_TXT;
    popup[IDX_RPAGE].type = TUI_TXT;
    if(dir_offset > 0){
        dir_lpage[0] = '<';
        popup[IDX_LPAGE].type = TUI_SEL;
    }
    if(dir_offset+DIR_PAGE_SIZE < dir_entries){
        dir_rpage[0] = '>';
        popup[IDX_RPAGE].type = TUI_SEL;
    }
    
}

uint8_t update_dir_ui(void){
    uint8_t dir_ok;
    dir_needs_refresh = true;
    dir_ok = dir_fill(loci_cfg.path);
    parse_files_to_widget();    
    tui_draw(popup);
    if(dir_entries)
        tui_set_current(POPUP_FILE_START);
    if(!dir_ok){
        tui_draw(warning);
    }
    return dir_ok;
}

int8_t calling_widget = -1;

void boot(bool do_return){
    char* boot_text;
    if(do_return && !return_possible)
        return;
    if(do_return)
        boot_text = (char*)&txt_returning;
    else
        boot_text = (char*)&txt_booting;
    tui_cls(3);
    strcpy(TUI_SCREEN_XY_CONST(17,14),boot_text);
    loci_cfg.tui_pos = tui_get_current();
    persist_set_loci_cfg(&loci_cfg);
    persist_set_magic();
    mia_set_ax(0x80 | (loci_cfg.ald_on <<4) | (loci_cfg.bit_on <<3) | (loci_cfg.b11_on <<2) | (loci_cfg.tap_on <<1) | loci_cfg.fdc_on);
    //mia_set_ax(0x00 | (loci_cfg.b11_on <<2) | (loci_cfg.tap_on <<1) | loci_cfg.fdc_on);
    VIA.ier = 0x7F;         //Disable VIA interrupts
    if(do_return)
        mia_restore_state();
    else{
        mia_clear_restore_buffer();
        mia_call_int_errno(MIA_OP_BOOT);    //Only returns if boot fails
    }
    VIA.ier = 0xC0;
    tui_cls(3);
    tui_draw(ui);
    DBG_STATUS("!ROM");
}

void update_onoff_btn(uint8_t idx, uint8_t on){
    if(on){
        tui_set_data(idx,txt_on);
    }else{
        tui_set_data(idx,txt_off);
    }
    tui_draw_widget(idx);
}

void update_mode_btn(){
    if(loci_cfg.bit_on){
        tui_set_data(IDX_TAP_BIT,txt_bit);
    }else{
        tui_set_data(IDX_TAP_BIT,txt_auto);
    }
    tui_draw_widget(IDX_TAP_BIT);
}

void update_rom_btn(){
    if(loci_cfg.mounts & (1u << 5)){
        tui_set_data(IDX_ROM,txt_custom);
        tui_set_type(IDX_ROM,TUI_TXT);
        tui_draw_widget(IDX_ROM);
    }else{
        if(loci_cfg.b11_on){
            tui_set_data(IDX_ROM,txt_basic11);
        }else{
            tui_set_data(IDX_ROM,txt_basic10);
        }
        tui_set_type(IDX_ROM,TUI_BTN);
        tui_draw_widget(IDX_ROM);
        tui_toggle_highlight(IDX_ROM);
    }
}

void update_eject_btn(uint8_t drv){
    tui_widget* widget;
    uint8_t idx;
    idx = tui_eject_idx[drv];
    widget = &ui[idx];
    if(loci_cfg.mounts & (1u << drv)){
        widget->type = TUI_SEL;
        tui_draw_widget(idx);
    }else{
        tui_clear_txt(idx);
        widget->type = TUI_NOP;
    }
}

void update_load_btn(){
    if(loci_cfg.ald_on){
        tui_set_data(IDX_TAP_LOAD,txt_auto);
    }else{
        tui_set_data(IDX_TAP_LOAD,txt_cload);
    }
    tui_draw_widget(IDX_TAP_LOAD);
}

void update_tap_counter(void){
    sprintf(txt_cnt,"%7lu",tap_tell());
    tui_draw_widget(IDX_TAP_CNT);
}

void do_eject(uint8_t drv, uint8_t ui_idx){
    umount(drv);
    loci_cfg.drv_names[drv][0] = '\0';
    tui_clear_txt(ui_idx);
    tui_draw_widget(ui_idx);
    loci_cfg.mounts &= ~(1u << drv);
    update_eject_btn(drv);
}

/* Ajuste le délai MAP (rv1) d'un cran, retune et redessine (factorisé : 4 appels) */
void map_step(int8_t d)
{
    if(d > 0 ? rv1 < 31 : rv1 > 0)
        rv1 += d;
    rv1 = tune_tmap(rv1);
    sprintf(txt_rv1, "%02d", rv1);
    tui_draw_widget(IDX_MAP_RV1);
}

void DisplayKey(unsigned char key)
{
    static unsigned char y = 0;
    char* screen, oscreen;
    char* tmp_ptr;
    char* ret;
    int drive;
    static uint8_t dir_ok = 1;
    uint8_t idx;
    uint8_t len;

    screen = (char*)(0xbb80+40*20+1);
    //oscreen = (char*)(0xbb80+40*21);
    //screen[0] = 16+7;
    //screen[1] = 0+4;
    switch(key){
        case(KEY_DELETE):
            idx = tui_get_current();
            if(tui_get_type(idx) == TUI_INP){
                tmp_ptr = (char*)tui_get_data(idx);
                len = strlen(tmp_ptr);
                if(len){
                    tmp_ptr[len-1] = '\0';
                    tui_draw_widget(idx);
                    tui_toggle_highlight(idx);
                }
            }else{
                if(calling_widget == -1){
                    switch(tui_get_current()){
                        case(IDX_DF0):
                        case(IDX_DF1):
                        case(IDX_DF2):
                        case(IDX_DF3):
                            /* IDX_DFn = IDX_DF0 + 2n (factorisé : ROM pleine) */
                            idx = tui_get_current();
                            do_eject((idx-IDX_DF0)>>1,idx);
                            tui_toggle_highlight(idx);
                            break;
                        case(IDX_TAP):
                            do_eject(4,IDX_TAP);
                            update_tap_counter();
                            tui_toggle_highlight(IDX_TAP);
                            break;
                        case(IDX_ROM_FILE):
                            do_eject(5,IDX_ROM_FILE);
                            tui_toggle_highlight(IDX_ROM_FILE);
                            update_rom_btn();
                            break;
                    }
                }else{
                    if(dir_ok && idx > POPUP_FILE_START && loci_cfg.path[0]=='0'){
                        tmp_ptr = (char*)tui_get_data(idx);
                        tmp_ptr[0]='/';
                        len = strlen(tmp_ptr);
                        do{
                            mia_push_char(tmp_ptr[--len]);
                        }while(len);
                        tmp_ptr[0]=' ';
                        len = strlen(loci_cfg.path);
                        do{
                            mia_push_char(loci_cfg.path[--len]);
                        }while(len);
                        if(mia_call_int_errno(MIA_OP_UNLINK)<0)
                            sprintf(TUI_SCREEN_XY_CONST(37,1),"%02x",errno);
                        dir_ok = dir_fill(loci_cfg.path);
                        parse_files_to_widget();
                        tui_draw(popup);
                    }
                }                          
            }
            break;
        case(KEY_UP):
            tui_prev_active();
            break;
        case(KEY_LEFT):
            if(calling_widget != -1){
                if(dir_ok){
                    dir_offset -= DIR_PAGE_SIZE;
                    parse_files_to_widget();
                    tui_draw(popup);
                    if(dir_entries)
                        tui_set_current(POPUP_FILE_START);
                }
            }
            break;
        case(KEY_DOWN):
            tui_next_active();
            break;
        case(KEY_RIGHT):
            if(calling_widget != -1){
                if(dir_ok){
                    dir_offset += DIR_PAGE_SIZE;
                    parse_files_to_widget();
                    tui_draw(popup);
                    if(dir_entries)
                        tui_set_current(POPUP_FILE_START);
                }
            }
            break;
        case('+'):
            if(calling_widget == -1){
                switch(tui_get_current()){
                    case(IDX_MAP_REW):
                    case(IDX_MAP_RV1):
                    case(IDX_MAP_FFW):
                        map_step(1);
                    break;
                }
            }
            break;
        case('_'):  //unshifted '-'
            if(calling_widget == -1){
                switch(tui_get_current()){
                    case(IDX_MAP_REW):
                    case(IDX_MAP_RV1):
                    case(IDX_MAP_FFW):
                        map_step(-1);
                    break;
                }
            }
            break;
        case(KEY_SPACE):
            //Exit from warning
            if(!dir_ok){    
                dir_ok = 1;
                tui_clear_box(1);
                tui_draw(popup);
                tui_set_current(POPUP_FILE_START);
                break;
            }
            if(calling_widget == -1){
                switch(tui_get_current()){
                    case(IDX_FDC_ON):
                        loci_cfg.fdc_on ^= 0x01;
                        update_onoff_btn(IDX_FDC_ON,loci_cfg.fdc_on);
                        tui_toggle_highlight(IDX_FDC_ON);
                        break;
                    case(IDX_MOU_ON):
                        loci_cfg.mou_on ^= 0x01;
                        update_onoff_btn(IDX_MOU_ON,loci_cfg.mou_on);
                        tui_toggle_highlight(IDX_MOU_ON);
                        if(loci_cfg.mou_on)
                            xreg_mia_mouse(0x7000);
                        else
                            xreg_mia_mouse(0xFFFF);
                        break;
                    case(IDX_TAP_ON):
                        loci_cfg.tap_on ^= 0x01;
                        update_onoff_btn(IDX_TAP_ON,loci_cfg.tap_on);
                        tui_toggle_highlight(IDX_TAP_ON);
                        break;
                    case(IDX_TAP_BIT):
                        loci_cfg.bit_on ^= 0x01;
                        update_mode_btn();
                        tui_toggle_highlight(IDX_TAP_BIT);
                        break;
                    case(IDX_TAP_LOAD):
                        loci_cfg.ald_on ^= 0x01;
                        update_load_btn();
                        tui_toggle_highlight(IDX_TAP_LOAD);
                        break; 
                    case(IDX_ROM):
                        loci_cfg.b11_on ^= 0x01;
                        update_rom_btn();
                        tui_toggle_highlight(IDX_ROM);
                        break;
                    case(IDX_DF0):
                    case(IDX_DF1):
                    case(IDX_DF2):
                    case(IDX_DF3):
                    case(IDX_TAP):
                    case(IDX_ROM_FILE):
                        calling_widget = tui_get_current();
                        tui_toggle_highlight(calling_widget);
                        if(calling_widget <= IDX_DF3)
                            strcpy(filter,".dsk");
                        else if(calling_widget == IDX_TAP)
                            strcpy(filter,".tap");
                        else
                            strcpy(filter,".rom");
                        popup[IDX_PATH].data = (char*)&loci_cfg.path;
                        dir_ok = update_dir_ui();
                        break;
                    case(IDX_TAP_CNT):
                        calling_widget = tui_get_current();
                        filter[0] = '\0';
                        dir_ok = tap_fill();
                        parse_files_to_widget();
                        popup[IDX_PATH].data = (char*)&loci_cfg.drv_names[4];
                        tui_draw(popup);
                        if(dir_entries)
                            tui_set_current(POPUP_FILE_START);
                        dir_needs_refresh = true;
                        break;
                    case(IDX_RETURN):
                        boot(true);
                        break;
                    case(IDX_BOOT):
                        boot(false);
                        break;
                    case(IDX_MAP_REW):
                        map_step(-1);
                        break;
                    case(IDX_MAP_FFW):
                        map_step(1);
                        break;
                    case(IDX_EJECT_DF0):
                    case(IDX_EJECT_DF1):
                    case(IDX_EJECT_DF2):
                    case(IDX_EJECT_DF3):
                        /* IDX_EJECT_DFn consécutifs, IDX_DFn = IDX_DF0 + 2n */
                        idx = tui_get_current() - IDX_EJECT_DF0;
                        do_eject(idx, IDX_DF0 + (idx<<1));
                        tui_set_current(IDX_DF0 + (idx<<1));
                        break;
                    case(IDX_EJECT_TAP):
                        do_eject(4,IDX_TAP);
                        update_tap_counter();
                        tui_set_current(IDX_TAP);
                        break;
                    case(IDX_EJECT_ROM):
                        do_eject(5,IDX_ROM_FILE);
                        tui_set_current(IDX_ROM_FILE);
                        update_rom_btn();
                        break;
                    case(IDX_TAP_REW):
                        TAP.cmd = TAP_CMD_REW;
                        update_tap_counter();
                        break;
                    case(IDX_TAP_FFW):
                        TAP.cmd = TAP_CMD_FFW;
                        update_tap_counter();
                        break;
                }
            }else{
                switch(tui_get_current()){
                    //Control elements selected
                    case(IDX_XPAGE):
                        tui_clear_box(1);
                        tui_draw(ui);
                        tui_set_current(calling_widget);
                        calling_widget = -1;
                        break;
                    case(IDX_LPAGE):
                        dir_offset -= DIR_PAGE_SIZE;
                        parse_files_to_widget();
                        tui_draw(popup);
                        if(dir_entries)
                            tui_set_current(POPUP_FILE_START);
                        break;
                    case(IDX_RPAGE):
                        dir_offset += DIR_PAGE_SIZE;
                        parse_files_to_widget();
                        tui_draw(popup);
                        if(dir_entries)
                            tui_set_current(POPUP_FILE_START);
                        break;
                    case(IDX_FILTER):
                        tmp_ptr = (char*)tui_get_data(idx);
                        len = strlen(tmp_ptr);
                        if(len < (tui_get_len(idx)-1)){
                            tmp_ptr[len] = key;
                            tmp_ptr[len+1] = '\0';
                            tui_draw_widget(idx);
                            tui_toggle_highlight(idx);
                        }
                        break;
                    default:
                        //Selection from the list
                        tmp_ptr = (char*)tui_get_data(tui_get_current());
                        if(tmp_ptr[0]=='/' || tmp_ptr[0]=='['){    //Directory or device selection
                            if(tmp_ptr[0]=='['){
                                loci_cfg.path[0] = tmp_ptr[1];
                                loci_cfg.path[1] = tmp_ptr[2];
                                loci_cfg.path[2] = 0x00;
                            }else if(tmp_ptr[1]=='.'){              //Go back down (/..)
                                if((ret = strrchr(loci_cfg.path,'/')) != NULL){
                                    ret[0] = 0x00;
                                }else{
                                    loci_cfg.path[0] = 0x00;
                                }
                            }else{
                                strncat(loci_cfg.path,tmp_ptr,256-strlen(loci_cfg.path));
                            }
                            dir_ok = update_dir_ui();
                            break;
                        }
                        //File selection
                        tmp_ptr = tmp_ptr + 1;      //adjust for leading space
                        switch(calling_widget){
                            case(IDX_DF0):
                                drive = 0;
                                break;
                            case(IDX_DF1):
                                drive = 1;
                                break;
                            case(IDX_DF2):
                                drive = 2;
                                break;
                            case(IDX_DF3):
                                drive = 3;
                                break;
                            case(IDX_TAP):
                                drive = 4;
                                break;
                            case(IDX_ROM_FILE):
                                drive = 5;
                                break;
                            case(IDX_TAP_CNT):
                                drive = 6;      //Pseudo drive
                        }
                        if(drive<6)
                            if(mount(drive,loci_cfg.path,tmp_ptr)==0x00){
                                loci_cfg.mounts |= 1u << drive;
                            }else{
                                loci_cfg.mounts &= ~1u << drive;
                            }
                        else
                            tap_seek(*((long*)(tmp_ptr-4-1)));  //Seek to start of header

                        tui_clear_box(1);
                        tui_draw(ui);
                        if(drive<6){
                            update_eject_btn(drive);
                            update_rom_btn();
                            tui_clear_txt(calling_widget);
                            strncpy(loci_cfg.drv_names[drive],tmp_ptr,32);
                            tui_set_data(calling_widget,loci_cfg.drv_names[drive]);
                            tui_draw_widget(calling_widget);
                            tui_set_current(calling_widget);
                        }
                        calling_widget = -1;
                        if(drive < 4){
                            loci_cfg.fdc_on = 0x01;
                            tui_set_data(IDX_FDC_ON,txt_on);
                            tui_draw_widget(IDX_FDC_ON);
                        }
                        if(drive == 4){
                            loci_cfg.tap_on = 0x01;
                            update_onoff_btn(IDX_TAP_ON,loci_cfg.tap_on);
                            loci_cfg.ald_on = 0x01;
                            update_load_btn();
                            update_tap_counter();
                        }
                        if(drive == 6){
                            update_tap_counter();
                        }
                }
            }
            break;
        case(KEY_RETURN):
            if(tui_get_type(tui_get_current()) == TUI_INP){
                dir_ok = dir_fill(loci_cfg.path);
                parse_files_to_widget();
                tui_draw(popup);
                if(!dir_ok){
                    tui_draw(warning);
                }
            }
            if(calling_widget == -1){
                boot(true);
            }
            //screen[y++] = 0x00;
            //dir_fill(screen);
            //if(strisquint(screen,filter)){
            //    DBG_STATUS("HIT ");
            //}else{
            //    DBG_STATUS("MISS");
            //}
            //write(STDOUT_FILENO, screen, y);
            //write(STDOUT_FILENO, '\n', 1);
            //read(STDIN_FILENO, oscreen, 280);
            break;
        case(KEY_ESCAPE):
            if(!dir_ok){
                dir_ok = 1;
                tui_clear_box(1);
                tui_draw(popup);
                break;
            }
            if(calling_widget == -1){   //Return to Oric
                boot(false);
            }else{                      //Escape from popup
                tui_clear_box(1);
                tui_draw(ui);
                tui_set_current(calling_widget);
                calling_widget = -1;
            }
            break;

        default:
            idx = tui_get_current();
            if(tui_get_type(idx) == TUI_INP){
                tmp_ptr = (char*)tui_get_data(idx);
                len = strlen(tmp_ptr);
                if(len < (tui_get_len(idx)-1)){
                    tmp_ptr[len] = key;
                    tmp_ptr[len+1] = '\0';
                    tui_draw_widget(idx);
                    tui_toggle_highlight(idx);
                }
            }else{  
                //Main menu keyboard shortcuts
                if(calling_widget == -1){
                    switch(key){
                        case('a'):
                            tui_set_current(IDX_DF0);
                            break;
                        case('b'):
                            tui_set_current(IDX_DF1);
                            break;
                        case('c'):
                            tui_set_current(IDX_DF2);
                            break;
                        case('d'):
                            tui_set_current(IDX_DF3);
                            break;
                        case('t'):
                            tui_set_current(IDX_TAP);
                            break;
                        case('k'):
                            tui_set_current(IDX_TAP_CNT);
                            break;
                        case('m'):
                            tui_set_current(IDX_MOU_ON);
                            break;
                        case('o'):
                            tui_set_current(IDX_ROM);
                            break;
                        case('r'):
                            tui_set_current(IDX_MAP_REW);
                            break;
                        case('s'):
                            sprintf(&txt_tior[5],"%02d",auto_tune_tior());
                            tui_draw_widget(IDX_TIOR);
                            break;
                        case('n'):
                            /* save-state : écrit l'état gelé dans 0:/LOCI.SNP
                             * (snapshot_save() refuse s'il n'y a pas de programme gelé) */
                            DBG_STATUS(snapshot_save()==0 ? "SNAP" : "!SNP");
                            break;
                        case('l'):
                            /* save-state : recharge 0:/LOCI.SNP puis reprend le programme */
                            if(snapshot_load()==0){
                                return_possible = true;   /* $B0 a rempli le tampon restore */
                                boot(true);
                            }else
                                DBG_STATUS("!SNP");
                            break;
                        case('w'):
                            /* loci-webdisk: monte le champ path (URL http://) en
                             * lecteur A. mount() recolle path + '/' + filename,
                             * donc on scinde l'URL sur son dernier '/'. */
                            tmp_ptr = strrchr(loci_cfg.path,'/');
                            if(tmp_ptr){
                                *tmp_ptr = '\0';
                                if(mount(0,loci_cfg.path,tmp_ptr+1)==0)
                                    loci_cfg.mounts |= 1u;
                                *tmp_ptr = '/';
                            }
                            break;
                    }
                }else{
                    //Directory popup keyboard shortcuts
                    if(dir_ok){
                        switch(key){
                            case('f'):
                                tui_set_current(IDX_FILTER);
                                break;
                            case('i'):
                                if(idx > POPUP_FILE_START && loci_cfg.path[0]!='0'){
                                    tmp_ptr = (char*)tui_get_data(idx);
                                    tmp_ptr[0] = '/';
                                    len = strlen(loci_cfg.path);
                                    strncat(loci_cfg.path,tmp_ptr,256-len);
                                    tmp_str[0] = '0';
                                    tmp_str[1] = ':';
                                    strcpy(&tmp_str[2],tmp_ptr);
                                    strcpy(dbg_status,"COPY");
                                    file_copy(tmp_str,loci_cfg.path);
                                    strcpy(dbg_status,"    ");
                                    tmp_ptr[0] = ' ';
                                    loci_cfg.path[len] = '\0';
                                }
                                break;
                            case('?'):  //Unshifted '/'
                                if((ret = strrchr(loci_cfg.path,'/')) != NULL){
                                    ret[0] = 0x00;
                                }else{
                                    loci_cfg.path[0] = 0x00;
                                }
                                dir_ok = update_dir_ui();
                                break;
                        }
                    }
                }
                //screen[ 0 + y++] = key;
            }
    }

    if(y>35) 
        y = 0;
}

unsigned char Mouse(unsigned char key){
    static uint16_t prev_pos = 0;
    static int8_t sx = 0, sy = 0;
    static uint8_t  prev_x = 0, prev_y = 0, prev_btn = 0, cursor = 0; 
    
    uint16_t pos;
    uint8_t x,y,btn;
    char *screen;
    uint8_t widget;

    if(!loci_cfg.mou_on)
        return key;
    
    screen = TUI_SCREEN;
    MIA.addr0 = 0x7000;
    MIA.step0 = 1;
    btn = MIA.rw0;
    x = MIA.rw0;
    y = MIA.rw0;
    sx = sx + (((int8_t)(x - prev_x))>>3);
    sy = sy + (((int8_t)(y - prev_y))>>3);
    if(sx >= TUI_SCREEN_W)
        sx = TUI_SCREEN_W-1;
    if(sx < 0)
        sx = 0;
    if(sy >= TUI_SCREEN_H)
        sy = TUI_SCREEN_H-1;
    if(sy < 0)
        sy = 0;
    pos = (TUI_SCREEN_W * sy) + sx;
    if(pos != prev_pos){
        if(cursor)
            screen[prev_pos] ^= 0x80;
        cursor = 1;
        screen[pos] ^= 0x80;
        prev_pos = pos;
        prev_x = x;
        prev_y = y;
    }
    
    if(((btn ^ prev_btn) & btn & 0x01)){  //Left mouse button release
        widget = tui_hit(sx,sy);
        if(widget){
            cursor = 0;
            tui_set_current(widget);
            key = KEY_SPACE;
        }
    } 
    prev_btn = btn;
    return key;
}

uint8_t auto_tune_tior(void){
    uint8_t i;
    static uint8_t ch;
    DBG_STATUS("    ");
    for(i=0; i<32; i++)
        TUI_PUTC(2+i,25,18);
    TUI_PUTC_CONST(2+32,25,16);
    TUI_PUTC_CONST(2+33,25,0);
    tune_scan_enable();
    while(!(loci_tior & 0x80)){}    //Wait for scan to begin
    while(!!(loci_tior & 0x80)){    //Scanning in progress
        i = (loci_tior & 0x7F);      //Using tior value as test pattern and map index
        if(i>31){
            DBG_STATUS("TADR");
            return i;
        }
        mia_push_char(i);
        mia_push_char('A');
        ch = mia_pop_char();        //Assignment needed to force read
        if(mia_pop_char() != i)
            TUI_PUTC(2+i,25,17);
    }
    i = (uint8_t)(strcasestr(TUI_SCREEN_XY_CONST(2, 25), "\x12\x12\x12\x12") - TUI_SCREEN_XY_CONST(2, 25) + 4);
    TUI_PUTC(2+i, 24, 'v');
    tune_tior(i);
    return i;
}

void main(void){
    uint8_t i;

    memcpy(popup, popup_tpl, sizeof popup_tpl);
    tui_cls(3);
    init_display();
    
    #ifdef VERSION
    sprintf(txt_title,"LOCI ROM %d.%d.%d FW %d.%d.%d",
        locirom_version[2],locirom_version[1],locirom_version[0],
        locifw_version[2], locifw_version[1], locifw_version[0]);
    #else
    sprintf(txt_title,"LOCI " __DATE__ " FW %d.%d.%d",
        locifw_version[2], locifw_version[1], locifw_version[0]);
    #endif

    return_possible = mia_restore_buffer_ok();

    if(!persist_get_loci_cfg(&loci_cfg)){
        loci_cfg.fdc_on = 0x00;
        loci_cfg.tap_on = 0x00;
        loci_cfg.bit_on = 0x00;
        loci_cfg.mou_on = 0x00;
        loci_cfg.b11_on = 0x01;
        loci_cfg.ser_on = 0x01;
        loci_cfg.ald_on = 0x00;
        loci_cfg.mounts = 0x00;
        loci_cfg.path[0] = 0x00;
        loci_cfg.drv_names[0][0] = 0x00;
        loci_cfg.drv_names[1][0] = 0x00;
        loci_cfg.drv_names[2][0] = 0x00;
        loci_cfg.drv_names[3][0] = 0x00;
        loci_cfg.drv_names[4][0] = 0x00;
        loci_cfg.drv_names[5][0] = 0x00;
        loci_cfg.tui_pos = IDX_DF0;
    }
   
    rv1 = loci_tmap;
    sprintf(txt_rv1,"%02d",rv1);
    sprintf(&txt_tior[5],"%02d",loci_tior);
    sprintf(&txt_tiow[5],"%02d",loci_tiow);
    sprintf(&txt_tiod[5],"%02d",loci_tiod);
    sprintf(&txt_tadr[5],"%02d",loci_tadr);
    tui_draw(ui);
    update_onoff_btn(IDX_FDC_ON,loci_cfg.fdc_on);
    update_onoff_btn(IDX_TAP_ON,loci_cfg.tap_on);
    update_onoff_btn(IDX_MOU_ON,loci_cfg.mou_on);
    update_load_btn();
    update_mode_btn();
    update_rom_btn();
    update_tap_counter();
    if(return_possible){
        tui_set_type(IDX_RETURN, TUI_SEL);
        tui_draw_widget(IDX_RETURN);
        i = mia_get_vmode();
        //sprintf(TUI_SCREEN_XY_CONST(32,0),"%02x",i);
        if(i & 0x04)
            TUI_PUTC_CONST(34,0,'H');   
        else
            TUI_PUTC_CONST(34,0,'T');
        if(i & 0x02)
            TUI_PUTC_CONST(35,0,'5');   
        else
            TUI_PUTC_CONST(35,0,'6');
        TUI_PUTC_CONST(36,0,'0');
    }

    for(i=0; i<=5; i++)
        update_eject_btn(i);
    tui_set_current(loci_cfg.tui_pos);
    dir_needs_refresh = 1;
    //sprintf((char*)(0xBB81+40),"%04X",tui_screen_xy(1,1));
    //dir_fill(path);
    //parse_files_to_widget();
    //tui_draw(popup);
    //tui_draw_box(10,28);
    
    InitKeyboard();

    while(1){
        char kb;
        unsigned char key;
        key = ReadKeyNoBounce();
        if(key && tui_get_type(tui_get_current()) == TUI_INP){ 
            key = ReadKey();
        }
        key = Mouse(key);
        if(key)
            DisplayKey(key);
        TUI_PUTC_CONST(39,1,txt_spinner[(irq_ticks & 0x03)]);
        
        kb = 0;
        i = 7;
        do{
            kb |= KeyMatrix[i];
        }while(i--);
        sprintf(TUI_SCREEN_XY_CONST(37,0),"%02x", kb);
    }
}
