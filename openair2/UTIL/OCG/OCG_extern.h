#ifndef __OCG_EXTERN_H__
#define __OCG_EXTERN_H__

extern char filename[]; /*!< \brief user_name.file_date.xml */
extern char user_name[]; /*!< \brief user_name  */
extern char file_date[]; /*!< \brief file_date */
extern char src_file[]; /*!< \brief USER_XML_FOLDER/user_name.file_date.xml or DEMO_XML_FOLDER/user_name.file_date.xml */
extern char dst_dir[]; /*!< \brief user_name/file_date/ */
extern int copy_or_move; /*!< \brief indicating if the current emulation is with a local XML or an XML generated from the web portal */
extern int file_detected; /*!< \brief indicate whether a new file is detected */
/* @}*/

/** @defgroup _oks OCG Module State Indicators
 *  @ingroup _OCG
 *  @brief Indicate whether a module has processed successfully
 * @{*/
extern int get_opt_OK; /*!< \brief value: -9999, -1, 0 or 1 */
extern int detect_file_OK; /*!< \brief value: -9999, -1 or 0 */
extern int parse_filename_OK; /*!< \brief value: -9999, -1 or 0 */
extern int create_dir_OK; /*!< \brief value: -9999, -1 or 0 */
extern int parse_XML_OK; /*!< \brief value: -9999, -1 or 0 */
extern int save_XML_OK; /*!< \brief value: -9999, -1 or 0 */
extern int call_emu_OK; /*!< \brief value: -9999, -1 or 0 */
extern int config_mobi_OK; /*!< \brief value: -9999, -1 or 0 */
extern int generate_report_OK; /*!< \brief value: -9999, -1 or 0 */
/* @}*/

extern OAI_Emulation oai_emulation;

#endif
