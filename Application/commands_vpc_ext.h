
#ifndef UTILS_COMMANDS_VPC_EXT_H_
#define UTILS_COMMANDS_VPC_EXT_H_

/***********************************************************************/

// Ortak RX cevabı (tüm başarılı komutlardan sonra gelir)
 uint8_t COMMON_ACK[] = {0x55, 0xAA, 0x01, 0x00, 0x01, 0xF0};
 uint8_t COMMON_NACK[]= {0x55, 0xAA, 0x01, 0x01, 0x00, 0xF0};
/*
 * 	Received message Eg：55 aa 13 00 00 2e 00 17 0a 11 0e 30 02 01 8f 3c da 97 01 04 03 00 f4 f0
* 	Focal spot temperature：0e 30 means 3632/100 = 36.32℃
* 	Firmware version number：17 0a 11 means 231017
* 	Machine identification code：8f 3c da 97 means 2403130007
*
* 	Bu komut gönderildiğinde dönen cevap içerisinde
 */
 constexpr uint8_t serialNumber[] = {0x00, 0x00};
 constexpr uint8_t productNumber[] = {0x00, 0x01};
 constexpr uint8_t FPA_height[]={0x00, 0x03};
 constexpr uint8_t FPA_core_Temp[]={0x00, 0x04};
 constexpr uint8_t core_temp[]={0x00, 0x05};
 constexpr uint8_t settings_saver[] = {0x00, 0x11};
 constexpr uint8_t factory_reset[] = {0x00, 0x12};
 constexpr uint8_t NUC_oto_shutter_state[]= {0x00, 0x15};
 constexpr uint8_t NUC_manuel_shutter[]= {0x00, 0x16};
 constexpr uint8_t NUC_oto_shutter_period[]= {0x00, 0x17};
 constexpr uint8_t NUC_oto_shutter_temp_range[]= {0x00, 0x18};
 constexpr uint8_t zoom_setter[]= {0x00, 0x2A};
 constexpr uint8_t image_palette[]= {0x00, 0x2D};
 constexpr uint8_t image_flip_commands[] = {0x00, 0x30};
 constexpr uint8_t agc_mode[] = {0x00, 0x3A};
 constexpr uint8_t brightness[] = {0x00, 0x3C};
 constexpr uint8_t contrast[]= {0x00, 0x3B};
 constexpr uint8_t gamma_corr[]= {0x00, 0x3B};
 constexpr uint8_t DDE_direction_read[]={0x00,0x3E};
 constexpr uint8_t DDE_direction_ctrl[]={0x00,0x3F};
 constexpr uint8_t DDE_direction_grade[]={0x00,0x3F};
 constexpr uint8_t analog_video_pause_continue[]  = {0x00, 0x32};
 constexpr uint8_t img_filter_on_off[]  = {0x00, 0x31};
 constexpr uint8_t oto_nuc_temp[]={0x00,0x18};
 constexpr uint8_t contrast_step[]={0x00,0x40};
 constexpr uint8_t brightness_step[]={0x00,0x41};
 constexpr uint8_t gamm_corr[]={0x00,0x3D};
 constexpr uint8_t DDE_ctrl[]={0x00,0x3E};
 constexpr uint8_t DDE_grade[]={0x00,0x3F};

constexpr uint8_t serialNumber_new[] = {0x00, 0x00, 0x80};
constexpr uint8_t productNumber_new[] = {0x00, 0x00, 0x80};
constexpr uint8_t FPA_new[] = {0x00, 0x00, 0x80};
constexpr uint8_t core_temp_new[]={0xA0, 0x02, 0x80};
constexpr uint8_t settings_saver_new[]= {0x01, 0x00, 0x04};
constexpr uint8_t NUC_manuel_shutter_new[] = {0x02, 0x01, 0x08};
constexpr uint8_t NUC_oto_shutter_state_new[] = {0x01, 0x00, 0x07};
constexpr uint8_t NUC_oto_shutter_period_set_new_[] = {0x01, 0x00, 0x01};
uint8_t zoom_setter_new[]  = {0x02, 0x00, 0x06};
constexpr uint8_t image_palette_new[] = {0x02, 0x00, 0x04};
constexpr uint8_t image_flip_commands_new[]  = {0x02, 0x00, 0x05};
constexpr uint8_t agc_mode_new[] = {0x02, 0x02, 0x06};
uint8_t brightness_new[] = {0x02, 0x02, 0x1E};
uint8_t contrast_new[]= {0x02, 0x02, 0x1F};
constexpr uint8_t img_filter_on_off_new[]  = {0x02, 0x0D, 0x06};
constexpr uint8_t analog_video_pause_continue_new[] = {0x01, 0x00, 0x02};
constexpr uint8_t DDE_direction_grade_new[]={0x00,0x3F};
constexpr uint8_t img_mode_ctrl[]={0x00,0x00};
constexpr uint8_t NUC_oto_shutter_period_and_state_read_new[]={ 0x01, 0x00, 0x80};







// Read Commands
// X, Y and mirror flip flop read   Receive command Byte:19
// black hot/ white hot  Receive command Byte:20
constexpr uint8_t read_palette_mirror[]={0x02, 0x00, 0x80};
uint8_t read_brightness_contrast[]={0x02,0x04,0x80};


constexpr uint8_t read_zoom[]={0x01};
constexpr uint8_t read_agc[]={0x02};
constexpr uint8_t read_auto_nuc_temp[]={0x03};
constexpr uint8_t read_gamma_corr[]={0x04};
constexpr uint8_t read_DDE_ctrl[]={0x06};
constexpr uint8_t read_DDE_grade[]={0x07};




//For buttons
constexpr uint8_t zoom_x1_0_setter_tx_new[]  = {0x55, 0xAA, 0x07, 0x02, 0x00, 0x06, 0x00, 0x00, 0x00, 0x08, 0x0B, 0xF0};
constexpr uint8_t zoom_x2_0_setter_tx_new[]  = {0x55, 0xAA, 0x07, 0x02, 0x00, 0x06, 0x00, 0x00, 0x00, 0x10, 0x13, 0xF0};

constexpr uint8_t image_palette_white_hot_setter_new_tx[] ={0x55,0xAA,0x07,0x02,0x00,0x04,0x00,0x00,0x00,0x00,0x01,0xF0};
constexpr uint8_t image_palette_black_hot_setter_new_tx[] ={0x55,0xAA,0x07,0x02,0x00,0x04,0x00,0x00,0x00,0x09,0x08,0xF0};
constexpr uint8_t PARAM_SAVE_tx[]={0x55,0xAA,0x07,0x01,0x00,0x04,0x00,0x00,0x00,0x01,0x03,0xF0};
constexpr uint8_t NUC_shutter_setter_tx_new[] = {0x55,0xAA,0x07,0x02,0x01,0x08,0x00,0x00,0x00,0x01,0x0D,0xF0};
constexpr uint8_t brightness_setter_tx_new[]={0x55,0xAA,0x07,0x02,0x02,0x1E,0x00,0x00,0x00,0x03,0x1A,0xF0};

#endif /* VPC_EXT_*/
