
#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "modules/exceptions.h"

#include "utilities/gzip.h"

#include <QTimer>

#include <chrono>
#include <filesystem>

#include <iostream> // !!! REMOVE !!!


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    //////////////////
    //// GRAPHICS ////
    this->ui->setupUi(this);

    // initialize the color-schemes map
    this->TB_COLOR_SCHEMES = ColorSec::getColorSchemes();
    // initialize the colors map
    this->COLORS = ColorSec::getColors();

    // load the main font
    this->main_font_family = QFontDatabase::applicationFontFamilies(
        QFontDatabase::addApplicationFont(":/fonts/Metropolis")).at(0);
    // load the alternative font
    this->alternative_font_family = QFontDatabase::applicationFontFamilies(
        QFontDatabase::addApplicationFont(":/fonts/Hack")).at(0);
    // load the script font
    this->script_font_family = QFontDatabase::applicationFontFamilies(
        QFontDatabase::addApplicationFont(":/fonts/3270")).at(0);
    // initialize the fonts map
    this->FONTS.emplace( "main", QFont(
        this->main_font_family,
        this->font_size ) );
    this->FONTS.emplace( "main_italic", QFont(
        this->main_font_family,
        this->font_size,
        -1, true ) );
    this->FONTS.emplace( "main_bold", QFont(
        this->main_font_family,
        this->font_size,
        1 ) );
    this->FONTS.emplace( "main_big", QFont(
        this->main_font_family,
        this->font_size_big ) );
    this->FONTS.emplace( "main_small", QFont(
        this->main_font_family,
        this->font_size_small ) );
    this->FONTS.emplace( "alternative", QFont(
        this->alternative_font_family,
        this->font_size ) );
    this->FONTS.emplace( "script", QFont(
        this->script_font_family,
        this->font_size ) );

    // parent font for every tab
    this->ui->CrapTabs->setFont( this->FONTS.at( "main_big" ) );

    // WebServers buttons for the LogFiles
    this->ui->button_LogFiles_Apache->setFont( this->FONTS.at( "main" ) );
    this->ui->button_LogFiles_Nginx->setFont( this->FONTS.at(  "main" ) );
    this->ui->button_LogFiles_Iis->setFont( this->FONTS.at(    "main" ) );
    // TreeView for the LogFiles
    this->ui->checkBox_LogFiles_CheckAll->setFont( this->FONTS.at( "main_small" ) );
    this->ui->listLogFiles->setFont( this->FONTS.at( "main" ) );
    // TextBrowser for the LogFiles
    this->TB.setColorScheme( 1, this->TB_COLOR_SCHEMES.at( 1 ) );
    this->TB.setFontFamily( this->main_font_family );
    this->TB.setFont( this->FONTS.at( "main" ) );
    this->ui->textLogFiles->setFont( this->TB.getFont() );
    // MakeStats labels
    this->ui->label_MakeStats_Size->setFont(  this->FONTS.at( "main" ) );
    this->ui->label_MakeStats_Lines->setFont( this->FONTS.at( "main" ) );
    this->ui->label_MakeStats_Time->setFont(  this->FONTS.at( "main" ) );
    this->ui->label_MakeStats_Speed->setFont( this->FONTS.at( "main" ) );

    // StatsSpeed table
    this->ui->table_StatsSpeed->setFont( this->FONTS.at("main") );

    // adjust LogsList headers width
    this->ui->listLogFiles->header()->resizeSection(0,200);
    this->ui->listLogFiles->header()->resizeSection(1,100);

    // blocknote
    this->crapnote->setFont( this->FONTS.at( "main" ) );


    //////////////
    //// MENU ////
    // languages
    connect( this->ui->actionEnglish,  &QAction::triggered, this, &MainWindow::menu_actionEnglish_triggered  );
    connect( this->ui->actionEspanol,  &QAction::triggered, this, &MainWindow::menu_actionEspanol_triggered  );
    connect( this->ui->actionFrancais, &QAction::triggered, this, &MainWindow::menu_actionFrancais_triggered );
    connect( this->ui->actionItaliano, &QAction::triggered, this, &MainWindow::menu_actionItaliano_triggered );
    // tools
    connect( this->ui->actionBlockNote, &QAction::triggered, this, &MainWindow::menu_actionBlockNote_triggered );
    connect( this->ui->actionCheckUpdates, &QAction::triggered, this, &MainWindow::menu_actionCheckUpdates_triggered );


    /////////////////
    //// CONFIGS ////
    this->defineOSspec();
    this->readConfigs();
    if ( this->language != "en" ) {
        this->updateUiLanguage();
    }
    if ( this->window_theme_id != 0 ) {
        this->updateUiTheme();
    }


    ///////////////////
    //// POLISHING ////
    // set the default WS as the current one
    switch ( this->default_ws ) {
        case 11:
            this->ui->button_LogFiles_Apache->setFlat( false );
            this->ui->radio_ConfDefaults_Apache->setChecked( true );
            break;
        case 12:
            this->ui->button_LogFiles_Nginx->setFlat( false );
            this->ui->radio_ConfDefaults_Nginx->setChecked( true );
            break;
        case 13:
            this->ui->button_LogFiles_Iis->setFlat( false );
            this->ui->radio_ConfDefaults_Iis->setChecked( true );
            break;
        default:
            // shouldn't be here
            throw WebServerException( "Unexpected WebServer ID: "+std::to_string( this->default_ws ) );
    }
    this->craplog.setCurrentWSID( this->default_ws );


    // make the Configs initialize
    // window
    this->ui->checkBox_ConfWindow_Geometry->setChecked( this->remember_window );
    this->ui->box_ConfWindow_Theme->setCurrentIndex( this->window_theme_id );
    // dialogs
    this->ui->slider_ConfDialogs_General->setValue( this->dialogs_level );
    this->ui->slider_ConfDialogs_Logs->setValue( this->craplog.getDialogsLevel() );
    this->ui->slider_ConfDialogs_Stats->setValue( this->crapview.getDialogsLevel() );
    // text browser
    this->ui->box_ConfTextBrowser_Font->setCurrentText( this->TB.getFontFamily() );
    this->ui->checkBox_ConfTextBrowser_WideLines->setChecked( this->TB.getWideLinesUsage() );
    this->ui->box_ConfTextBrowser_ColorScheme->setCurrentIndex( this->TB.getColorSchemeID() );
    this->refreshTextBrowserPreview();
    // charts
    this->ui->box_ConfCharts_Theme->setCurrentIndex( this->CHARTS_THEMES.at( this->charts_theme_id ) );
    this->refreshChartsPreview();
    // databases
    this->ui->inLine_ConfDatabases_Data_Path->setText( QString::fromStdString( this->db_data_path ) );
    this->ui->button_ConfDatabases_Data_Save->setEnabled( false );
    this->ui->inLine_ConfDatabases_Hashes_Path->setText( QString::fromStdString( this->db_hashes_path ) );
    this->ui->button_ConfDatabases_Hashes_Save->setEnabled( false );
    // logs control
    this->ui->checkBox_ConfControl_Usage->setChecked( this->hide_used_files );
    this->ui->spinBox_ConfControl_Size->setValue( this->craplog.getWarningSize() / 1'048'576 );
    if ( this->craplog.getWarningSize() > 0 ) {
        this->ui->checkBox_ConfControl_Size->setChecked( true );
    } else {
        this->ui->checkBox_ConfControl_Size->setChecked( false );
    }
    // apache paths
    this->ui->inLine_ConfApache_Path_String->setText( QString::fromStdString(this->craplog.getLogsPath( this->APACHE_ID )) );
    this->ui->button_ConfApache_Path_Save->setEnabled( false );
    // apache formats
    this->ui->inLine_ConfApache_Format_String->setText( QString::fromStdString( this->craplog.getLogsFormatString( this->APACHE_ID ) ) );
    this->ui->button_ConfApache_Format_Save->setEnabled( false );
    // apache warnlists
    this->on_box_ConfApache_Warnlist_Field_currentTextChanged( this->ui->box_ConfApache_Warnlist_Field->currentText() );
    // apache blacklists
    this->on_box_ConfApache_Blacklist_Field_currentTextChanged( this->ui->box_ConfApache_Blacklist_Field->currentText() );
    // nginx paths
    this->ui->inLine_ConfNginx_Path_String->setText( QString::fromStdString(this->craplog.getLogsPath( this->NGINX_ID )) );
    this->ui->button_ConfNginx_Path_Save->setEnabled( false );
    // nginx formats
    this->ui->inLine_ConfNginx_Format_String->setText( QString::fromStdString( this->craplog.getLogsFormatString( this->NGINX_ID ) ) );
    this->ui->button_ConfNginx_Format_Save->setEnabled( false );
    // nginx warnlists
    this->on_box_ConfNginx_Warnlist_Field_currentTextChanged( this->ui->box_ConfNginx_Warnlist_Field->currentText() );
    // nginx blacklists
    this->on_box_ConfNginx_Blacklist_Field_currentTextChanged( this->ui->box_ConfNginx_Blacklist_Field->currentText() );
    // iis paths
    this->ui->inLine_ConfIis_Path_String->setText( QString::fromStdString(this->craplog.getLogsPath( this->IIS_ID )) );
    this->ui->button_ConfIis_Path_Save->setEnabled( false );
    // iis formats
    this->ui->inLine_ConfIis_Format_String->setText( QString::fromStdString( this->craplog.getLogsFormatString( this->IIS_ID ) ) );
    this->ui->button_ConfIis_Format_Save->setEnabled( false );
    // iis warnlists
    this->on_box_ConfIis_Warnlist_Field_currentTextChanged( this->ui->box_ConfIis_Warnlist_Field->currentText() );
    // iis blacklists
    this->on_box_ConfIis_Blacklist_Field_currentTextChanged( this->ui->box_ConfIis_Blacklist_Field->currentText() );


    // blocknote's font and colors
    this->crapnote->setTextFont( this->TB.getFont() );
    this->crapnote->setColorScheme( this->TB.getColorSchemeID() );

    // text browser's default message
    {
        QString rich_text;
        RichText::richLogsDefault( rich_text );
        this->ui->textLogFiles->setText( rich_text );
        rich_text.clear();
    }

    // get a fresh list of LogFiles
    this->craplog_timer = new QTimer(this);
    connect(this->craplog_timer, SIGNAL(timeout()), this, SLOT(wait_ActiveWindow()));
    this->craplog_timer->start(250);
}

MainWindow::~MainWindow()
{
    delete this->ui;
    delete this->craphelp;
    //delete this->translator;
}

void MainWindow::closeEvent (QCloseEvent *event)
{
    // save actual configurations
    this->writeConfigs();

    // save splitters sizes => this->ui->splitter_StatsCount->sizes();
}



////////////////////////
//// CONFIGURATIONS ////
////////////////////////
// os definition
void MainWindow::defineOSspec()
{
    const std::string home_path = StringOps::rstrip( QStandardPaths::locate( QStandardPaths::HomeLocation, "", QStandardPaths::LocateDirectory ).toStdString(), "/" );
    switch ( this->OS ) {
        case 1:
            // unix-like
            this->configs_path   = home_path + "/.config/LogDoctor/logdoctor.conf";
            this->logdoc_path    = home_path + "/.local/share/LogDoctor";
            this->db_data_path   = logdoc_path;
            this->db_hashes_path = logdoc_path;
            break;

        case 2:
            // windows
            this->configs_path   = home_path + "/AppData/Local/LogDoctor/logdoctor.conf";
            this->logdoc_path    = home_path + "/AppData/Local/LogDoctor";
            this->db_data_path   = logdoc_path;
            this->db_hashes_path = logdoc_path;
            break;

        case 3:
            // darwin-based
            this->configs_path   = home_path + "/Lybrary/Preferences/LogDoctor/logdoctor.conf";
            this->logdoc_path    = home_path + "/Lybrary/Application Support/LogDoctor";
            this->db_data_path   = logdoc_path;
            this->db_hashes_path = logdoc_path;
            break;

        default:
            // shouldn't be here
            throw GenericException( "Unexpected OS ID: "+std::to_string( this->OS ) );
    }
}

void MainWindow::readConfigs()
{
    bool proceed = true;
    // check the file
    if ( IOutils::exists( this->configs_path ) == true ) {
        if ( IOutils::checkFile( this->configs_path ) == true ) {
            if ( IOutils::checkFile( this->configs_path, true ) == false ) {
                // file not readable
                try {
                    std::filesystem::permissions( this->configs_path,
                                                  std::filesystem::perms::owner_read,
                                                  std::filesystem::perm_options::add );
                } catch (...) {
                    proceed = false;
                    QString file = "";
                    if ( this->dialogs_level > 0 ) {
                        file = QString::fromStdString( this->configs_path );
                    }
                    DialogSec::errConfFileNotReadable( nullptr, file );
                }
            }
        } else {
            // the given path doesn't point to a file
            proceed = DialogSec::choiceFileNotFile( nullptr, QString::fromStdString( this->configs_path ) );
            if ( proceed == true ) {
                proceed = IOutils::renameAsCopy( this->configs_path );
                if ( proceed == false ) {
                    QString path = "";
                    if ( this->dialogs_level > 0 ) {
                        path = QString::fromStdString( this->configs_path );
                    }
                    DialogSec::errRenaming( nullptr, QString::fromStdString( this->configs_path ) );
                }
            }
        }
    } else {
        // configuration file not found
        proceed = false;
        QString file = "";
        if ( this->dialogs_level == 2 ) {
            file = QString::fromStdString( this->configs_path );
        }
        DialogSec::warnConfFileNotFound( nullptr, file );
    }

    if ( proceed == true ) {
        std::vector<std::string> aux, list, configs;
        try {
            std::string content;
            IOutils::readFile( this->configs_path, content );
            StringOps::splitrip( configs, content );
            for ( const std::string& line : configs ) {
                if ( StringOps::startsWith( line, "[") == true ) {
                    // section descriptor
                    continue;
                }
                aux.clear();
                StringOps::splitrip( aux, line, "=" );
                if ( aux.size() < 2 ) {
                    // nothing to do
                    continue;
                }
                // if here, a value is present
                const std::string& var = aux.at( 0 ),
                                   val = aux.at( 1 );

                if ( val.size() == 0 ) {
                    // nothing to do, no value stored
                    continue;
                }

                if ( var == "Language" ) {
                    if ( val.size() > 2 ) {
                        // not a valid locale, keep the default
                        DialogSec::errLangLocaleInvalid( nullptr, QString::fromStdString( val ) );
                    } else {
                        if ( val == "en" || val == "es" || val == "fr" || val == "it" ) {
                            this->language = val;
                        } else {
                            DialogSec::errLangNotAccepted( nullptr, QString::fromStdString( val ) );
                        }
                    }

                } else if ( var == "RememberGeometry" ) {
                    this->remember_window = this->s2b.at( val );

                } else if ( var == "Geometry" ) {
                    this->geometryFromString( val );

                } else if ( var == "WindowTheme" ) {
                    this->window_theme_id = std::stoi( val );

                } else if ( var == "ChartsTheme" ) {
                    this->charts_theme_id = std::stoi( val );

                } else if ( var == "MainDialogLevel" ) {
                    this->dialogs_level = std::stoi( val );

                } else if ( var == "DefaultWebServer" ) {
                    this->default_ws = std::stoi( val );

                } else if ( var == "DatabaseDataPath" ) {
                    this->db_data_path = this->resolvePath( val );

                } else if ( var == "DatabaseHashesPath" ) {
                    this->db_hashes_path = this->resolvePath( val );

                } else if ( var == "Font" ) {
                    this->on_box_ConfTextBrowser_Font_currentIndexChanged( std::stoi( val ) );

                } else if ( var == "WideLines" ) {
                    this->TB.setWideLinesUsage( this->s2b.at( val ) );

                } else if ( var == "ColorScheme" ) {
                    this->on_box_ConfTextBrowser_ColorScheme_currentIndexChanged( std::stoi( val ) );

                } else if ( var == "CraplogDialogLevel" ) {
                    this->craplog.setDialogsLevel( std::stoi( val ) );

                } else if ( var == "HideUsedFiles" ) {
                    hide_used_files = this->s2b.at( val );

                } else if ( var == "WarningSize" ) {
                    this->craplog.setWarningSize( std::stoi( val ) );

                } else if ( var == "ApacheLogsPath" ) {
                    this->craplog.setLogsPath( this->APACHE_ID, this->resolvePath( val ) );

                } else if ( var == "ApacheLogsFormat" ) {
                    this->craplog.setApacheLogFormat( val );

                } else if ( var == "ApacheWarnlistMethod" ) {
                    this->craplog.setWarnlist( this->APACHE_ID, 11, this->string2list( val ) );

                } else if ( var == "ApacheWarnlistURI" ) {
                    this->craplog.setWarnlist( this->APACHE_ID, 12, this->string2list( val ) );

                } else if ( var == "ApacheWarnlistClient" ) {
                    this->craplog.setWarnlist( this->APACHE_ID, 20, this->string2list( val ) );

                } else if ( var == "ApacheWarnlistUserAgent" ) {
                    this->craplog.setWarnlist( this->APACHE_ID, 21, this->string2list( val ) );

                } else if ( var == "ApacheBlacklistClient" ) {
                    this->craplog.setBlacklist( this->APACHE_ID, 20, this->string2list( val ) );

                } else if ( var == "NginxLogsPath" ) {
                    this->craplog.setLogsPath( this->NGINX_ID, this->resolvePath( val ) );

                } else if ( var == "NginxLogsFormat" ) {
                    this->craplog.setNginxLogFormat( val );

                } else if ( var == "NginxWarnlistMethod" ) {
                    this->craplog.setWarnlist( this->NGINX_ID, 11, this->string2list( val ) );

                } else if ( var == "NginxWarnlistURI" ) {
                    this->craplog.setWarnlist( this->NGINX_ID, 12, this->string2list( val ) );

                } else if ( var == "NginxWarnlistClient" ) {
                    this->craplog.setWarnlist( this->NGINX_ID, 20, this->string2list( val ) );

                } else if ( var == "NginxWarnlistUserAgent" ) {
                    this->craplog.setWarnlist( this->NGINX_ID, 21, this->string2list( val ) );

                } else if ( var == "NginxBlacklistClient" ) {
                    this->craplog.setBlacklist( this->NGINX_ID, 20, this->string2list( val ) );

                } else if ( var == "IisLogsPath" ) {
                    this->craplog.setLogsPath( this->IIS_ID, this->resolvePath( val ) );

                } else if ( var == "IisLogsModule" ) {
                    if ( val == "1" ) {
                        this->ui->radio_ConfIis_Format_NCSA->setChecked( true );
                    } else if ( val == "2" ) {
                        this->ui->radio_ConfIis_Format_IIS->setChecked( true );
                    }

                } else if ( var == "IisLogsFormat" ) {
                    int module = 0;
                    if ( this->ui->radio_ConfIis_Format_NCSA->isChecked() == true ) {
                        module = 1;
                    } else if ( this->ui->radio_ConfIis_Format_IIS->isChecked() == true ) {
                        module = 2;
                    }
                    this->craplog.setIisLogFormat( val, module );

                } else if ( var == "IisWarnlistMethod" ) {
                    this->craplog.setWarnlist( this->IIS_ID, 11, this->string2list( val ) );

                } else if ( var == "IisWarnlistURI" ) {
                    this->craplog.setWarnlist( this->IIS_ID, 12, this->string2list( val ) );

                } else if ( var == "IisWarnlistClient" ) {
                    this->craplog.setWarnlist( this->IIS_ID, 20, this->string2list( val ) );

                } else if ( var == "IisWarnlistUserAgent" ) {
                    this->craplog.setWarnlist( this->IIS_ID, 21, this->string2list( val ) );

                } else if ( var == "IisBlacklistClient" ) {
                    this->craplog.setBlacklist( this->IIS_ID, 20, this->string2list( val ) );

                } else if ( var == "CrapviewDialogLevel" ) {
                    this->crapview.setDialogsLevel( std::stoi( val ) );

                }/* else {
                    // not valid
                }*/
            }
        } catch (const std::ios_base::failure& err) {
            // failed reading
            QString msg = QMessageBox::tr("An error occured while reading the configurations file");
            DialogSec::errGeneric( nullptr, msg );
        } catch (...) {
            // something failed
            QString msg = QMessageBox::tr("An error occured while parsing configuration data");
            DialogSec::errGeneric( nullptr, msg );
        }
    }
}

void MainWindow::writeConfigs()
{
    bool proceed=true, msg_shown=false;
    QString msg;
    // check the file first
    if ( IOutils::exists( this->configs_path ) == true ) {
        if ( IOutils::checkFile( this->configs_path ) == true ) {
            if ( IOutils::checkFile( this->configs_path, false, true ) == false ) {
                // file not writable
                try {
                    std::filesystem::permissions( this->configs_path,
                                                  std::filesystem::perms::owner_write,
                                                  std::filesystem::perm_options::add );
                } catch (...) {
                    proceed = false;
                    QString file = "";
                    if ( this->dialogs_level > 0 ) {
                        file = QString::fromStdString( this->configs_path );
                    }
                    DialogSec::errConfFileNotWritable( nullptr, file );
                    msg_shown = true;
                }
            }
        } else {
            // the given path doesn't point to a file
            proceed = DialogSec::choiceFileNotFile( nullptr, QString::fromStdString( this->configs_path ) );
            if ( proceed == true ) {
                proceed = IOutils::renameAsCopy( this->configs_path );
                if ( proceed == false ) {
                    QString path = "";
                    if ( this->dialogs_level > 0 ) {
                        path = QString::fromStdString( this->configs_path );
                    }
                    DialogSec::errRenaming( nullptr, path );
                    msg_shown = true;
                }
            }
        }
    } else {
        // file does not exists, check if at least the folder exists
        const std::string base_path = this->basePath( this->configs_path );
        if ( IOutils::exists( base_path ) == true ) {
            if ( IOutils::isDir( base_path ) == true ) {
                if ( IOutils::checkDir( base_path, false, true ) == false ) {
                    // directory not writable
                    try {
                        std::filesystem::permissions( base_path,
                                                      std::filesystem::perms::owner_write,
                                                      std::filesystem::perm_options::add );
                    } catch (...) {
                        proceed = false;
                        QString file = "";
                        if ( this->dialogs_level > 0 ) {
                            file = QString::fromStdString( base_path );
                        }
                        DialogSec::errConfDirNotWritable( nullptr, file );
                        msg_shown = true;
                    }
                }
            } else {
                // not a directory
                proceed = DialogSec::choiceDirNotDir( nullptr, QString::fromStdString( base_path ) );
                if ( proceed == true ) {
                    proceed = IOutils::renameAsCopy( base_path );
                    if ( proceed == false ) {
                        QString path = "";
                        if ( this->dialogs_level > 0 ) {
                            path = QString::fromStdString( base_path );
                        }
                        DialogSec::errRenaming( nullptr, path );
                        msg_shown = true;
                    }
                }
            }
        } else {
            // the folder does not exist too
            proceed = IOutils::makeDir( base_path );
            if ( proceed == false ) {
                msg = QMessageBox::tr("Unable to create the directory");
                if ( this->dialogs_level > 0 ) {
                    msg += ":\n"+QString::fromStdString( base_path );
                }
            }
        }
    }
    if ( proceed == false && msg_shown == false ) {
        DialogSec::errConfFailedWriting( nullptr, msg );
    }

    if ( proceed == true ) {
        //// USER INTERFACE ////
        std::string configs = "[UI]";
        configs += "\nLanguage=" + this->language;
        configs += "\nRememberGeometry=" + this->b2s.at( this->remember_window );
        configs += "\nGeometry=" + this->geometryToString();
        configs += "\nWindowTheme=" + std::to_string( this->window_theme_id );
        configs += "\nChartsTheme=" + std::to_string( this->charts_theme_id );
        configs += "\nMainDialogLevel=" + std::to_string( this->dialogs_level );
        configs += "\nDefaultWebServer=" + std::to_string( this->default_ws );
        configs += "\nDatabaseDataPath=" + this->db_data_path;
        configs += "\nDatabaseHashesPath=" + this->db_hashes_path;
        //// TEXT BROWSER ////
        configs += "\n\n[TextBrowser]";
        configs += "\nFont=" + std::to_string( this->ui->box_ConfTextBrowser_Font->currentIndex() );
        configs += "\nWideLines=" + this->b2s.at( this->TB.getWideLinesUsage() );
        configs += "\nColorScheme=" + std::to_string( this->TB.getColorSchemeID() );
        //// CRAPLOG ////
        configs += "\n\n[Craplog]";
        configs += "\nCraplogDialogLevel=" + std::to_string( this->craplog.getDialogsLevel() );
        configs += "\nHideUsedFiles=" + this->b2s.at( this->hide_used_files );
        configs += "\nWarningSize=" + std::to_string( this->craplog.getWarningSize() );
        //// APACHE2 ////
        configs += "\n\n[Apache2]";
        configs += "\nApacheLogsPath=" + this->craplog.getLogsPath( this->APACHE_ID );
        configs += "\nApacheLogsFormat=" + this->craplog.getLogsFormatString( this->APACHE_ID );
        configs += "\nApacheWarnlistMethod=" + this->list2string( this->craplog.getWarnlist( this->APACHE_ID, 11 ) );
        configs += "\nApacheWarnlistURI=" + this->list2string( this->craplog.getWarnlist( this->APACHE_ID, 12 ) );
        configs += "\nApacheWarnlistClient=" + this->list2string( this->craplog.getWarnlist( this->APACHE_ID, 20 ) );
        configs += "\nApacheWarnlistUserAgent=" + this->list2string( this->craplog.getWarnlist( this->APACHE_ID, 21 ), true );
        configs += "\nApacheBlacklistClient=" + this->list2string( this->craplog.getBlacklist( this->APACHE_ID, 20 ) );
        //// NGINX ////
        configs += "\n\n[Nginx]";
        configs += "\nNginxLogsPath=" + this->craplog.getLogsPath( this->NGINX_ID );
        configs += "\nNginxLogsFormat=" + this->craplog.getLogsFormatString( this->NGINX_ID );
        configs += "\nNginxWarnlistMethod=" + this->list2string( this->craplog.getWarnlist( this->NGINX_ID, 11 ) );
        configs += "\nNginxWarnlistURI=" + this->list2string( this->craplog.getWarnlist( this->NGINX_ID, 12 ) );
        configs += "\nNginxWarnlistClient=" + this->list2string( this->craplog.getWarnlist( this->NGINX_ID, 20 ) );
        configs += "\nNginxWarnlistUserAgent=" + this->list2string( this->craplog.getWarnlist( this->NGINX_ID, 21 ), true );
        configs += "\nNginxBlacklistClient=" + this->list2string( this->craplog.getBlacklist( this->NGINX_ID, 20 ) );
        //// IIS ////
        configs += "\n\n[IIS]";
        configs += "\nIisLogsPath=" + this->craplog.getLogsPath( this->IIS_ID );
        std::string module = "0";
        if ( this->ui->radio_ConfIis_Format_NCSA->isChecked() == true ) {
            module = "1";
        } else if ( this->ui->radio_ConfIis_Format_IIS->isChecked() == true ) {
            module = "2";
        }
        configs += "\nIisLogsModule=" + module;
        configs += "\nIisLogsFormat=" + this->craplog.getLogsFormatString( this->IIS_ID );
        configs += "\nIisWarnlistMethod=" + this->list2string( this->craplog.getWarnlist( this->IIS_ID, 11 ) );
        configs += "\nIisWarnlistURI=" + this->list2string( this->craplog.getWarnlist( this->IIS_ID, 12 ) );
        configs += "\nIisWarnlistClient=" + this->list2string( this->craplog.getWarnlist( this->IIS_ID, 20 ) );
        configs += "\nIisWarnlistUserAgent=" + this->list2string( this->craplog.getWarnlist( this->IIS_ID, 21 ), true );
        configs += "\nIisBlacklistClient=" + this->list2string( this->craplog.getBlacklist( this->IIS_ID, 20 ) );
        //// CRAPVIEW ////
        configs += "\n\n[Crapview]";
        configs += "\nCrapviewDialogLevel=" + std::to_string( this->crapview.getDialogsLevel() );

        // write on file
        try {
            IOutils::writeOnFile( this->configs_path, configs );

        } catch (const std::ios_base::failure& err) {
            // failed writing
            QString msg = QMessageBox::tr("An error occured while writing the configurations file");
            DialogSec::errGeneric( nullptr, msg );
        } catch (...) {
            // something failed
            QString msg = QMessageBox::tr("An error occured while preparing configurations data");
            DialogSec::errGeneric( nullptr, msg );
        }
    }
}


const std::string MainWindow::geometryToString()
{
    QRect geometry = this->geometry();
    std::string string = "";
    string += std::to_string( geometry.x() );
    string += ",";
    string += std::to_string( geometry.y() );
    string += ",";
    string += std::to_string( geometry.width() );
    string += ",";
    string += std::to_string( geometry.height() );
    string += ",";
    string += this->b2s.at( this->isMaximized() );
    return string;
}
void MainWindow::geometryFromString( const std::string& geometry )
{
    std::vector<std::string> aux;
    StringOps::splitrip( aux, geometry, "," );
    QRect new_geometry;
    new_geometry.setRect( std::stoi(aux.at(0)), std::stoi(aux.at(1)), std::stoi(aux.at(2)), std::stoi(aux.at(3)) );
    this->setGeometry( new_geometry );
    if ( aux.at(4) == "true" ) {
        this->showMaximized();
    }
}


const std::string MainWindow::list2string( const std::vector<std::string>& list, const bool& user_agent )
{
    int i, max=list.size()-1;
    std::string string;
    if ( user_agent == true ) {
        for ( const std::string& str : list ) {
            string += StringOps::replace( str, " ", "%@#" );
            if ( i < max ) {
                string.push_back( ' ' );
                i++;
            }
        }
    } else {
        for ( const std::string& str : list ) {
            string += str;
            if ( i < max ) {
                string.push_back( ' ' );
                i++;
            }
        }
    }
    return string;
}
const std::vector<std::string> MainWindow::string2list( const std::string& string, const bool& user_agent )
{
    std::vector<std::string> list, aux;
    StringOps::splitrip( aux, string, " " );
    if ( user_agent == true ) {
        for ( const std::string& str : aux ) {
            list.push_back( StringOps::replace( str, " ", "%@#" ) );
        }
    } else {
        for ( const std::string& str : aux ) {
            list.push_back( str );
        }
    }
    return list;
}


//////////////////
//// GRAPHICS ////
//////////////////
void MainWindow::updateUiTheme()
{
    if ( this->window_theme_id < 0 || this->window_theme_id > 2 ) {
        // wrong
        throw GenericException( "Unexpected WindowTheme ID: "+std::to_string( this->window_theme_id ) );
    }
    this->setPalette( ColorSec::getPalette( this->window_theme_id ) );
}


//////////////////
//// LANGUAGE ////
//////////////////
void MainWindow::updateUiLanguage()
{
    // remove the old translator
    QCoreApplication::removeTranslator( &this->translator );
    if ( this->translator.load( QString(":/translations/%1").arg(QString::fromStdString( this->language )) ) ) {
        // apply the new translator
        QCoreApplication::installTranslator( &this->translator );
        this->ui->retranslateUi( this );
    }
}


//////////////////////////
//// INTEGRITY CHECKS ////
//////////////////////////
void MainWindow::wait_ActiveWindow()
{
    if ( this->isActiveWindow() == false ) {
        std::this_thread::sleep_for( std::chrono::milliseconds(250) );
    } else {
        this->craplog_timer->stop();
        this->makeInitialChecks();
    }
}
void MainWindow::makeInitialChecks()
{
    bool ok = true;
    // check that the sqlite plugin is available
    if ( QSqlDatabase::drivers().contains("QSQLITE") == false ) {
        // checks failed, abort
        DialogSec::errSqlDriverNotFound( nullptr, "QSQLITE" );
        ok = false;
    }

    if ( ok == true ) {
        // check LogDoctor's folders paths
        for ( const std::string& path : std::vector<std::string>({this->basePath(this->configs_path), this->logdoc_path}) ) {
            if ( IOutils::exists( path ) == true ) {
                if ( IOutils::isDir( path ) == true ) {
                    if ( IOutils::checkDir( path, true ) == false ) {
                        try {
                            std::filesystem::permissions( path,
                                                          std::filesystem::perms::owner_read,
                                                          std::filesystem::perm_options::add );
                        } catch (...) {
                            ok = false;
                            DialogSec::errDirNotReadable( nullptr, QString::fromStdString( path ) );
                        }
                    }
                    if ( ok == true ) {
                        if ( IOutils::checkDir( path, false, true ) == false ) {
                            try {
                                std::filesystem::permissions( path,
                                                              std::filesystem::perms::owner_write,
                                                              std::filesystem::perm_options::add );
                            } catch (...) {
                                ok = false;
                                DialogSec::errDirNotWritable( nullptr, QString::fromStdString( path ) );
                            }
                        }
                    }

                } else {
                    // not a directory, rename as copy a make a new one
                    ok = DialogSec::choiceDirNotDir( nullptr, QString::fromStdString( path ) );
                    if ( ok == true ) {
                        ok = IOutils::renameAsCopy( path );
                        if ( ok == false ) {
                            QString p = "";
                            if ( this->dialogs_level > 0 ) {
                                p = QString::fromStdString( path );
                            }
                            DialogSec::errRenaming( nullptr, p );
                        }
                    }
                }

            } else {
                ok = IOutils::makeDir( path );
            }
            if ( ok == false ) {
                break;
            }
        }
    }

    if ( ok == true ) {
        // statistics' database
        if ( CheckSec::checkStatsDatabase( this->db_data_path + "/collection.db" ) == false ) {
            // checks failed, abort
            ok = false;
        } else {
            this->crapview.setDbPath( this->db_data_path );
            this->craplog.setStatsDatabasePath( this->db_data_path );
            // used-files' hashes' database
            if ( CheckSec::checkHashesDatabase( this->db_hashes_path + "/hashes.db" ) == false ) {
                // checks failed, abort
                ok = false;
            } else {
                this->craplog.setHashesDatabasePath( this->db_hashes_path );
                if ( this->craplog.hashOps.loadUsedHashesLists( this->db_hashes_path + "/hashes.db" ) == false ) {
                    // failed to load the list, abort
                    ok = false;
                } else {
                    // craplog variables
                    if ( CheckSec::checkCraplog( this->craplog ) == false ) {
                        // checks failed, abort
                        ok = false;
                    }
                }
            }
        }
    }
    if ( ok == false ) {
        this->close();
        //QCoreApplication::exit(0);
        //this->destroy();
    } else {
        // get available stats dates
        this->refreshStatsDates();
        // get a fresh list of log files
        this->on_button_LogFiles_RefreshList_clicked();
        // set the default WS as the current one
        switch ( this->craplog.getCurrentWSID() ) {
            case 11:
                this->ui->box_StatsWarn_WebServer->setCurrentIndex(  0 );
                this->ui->box_StatsCount_WebServer->setCurrentIndex( 0 );
                this->ui->box_StatsSpeed_WebServer->setCurrentIndex( 0 );
                this->ui->box_StatsDay_WebServer->setCurrentIndex(   0 );
                this->ui->box_StatsRelat_WebServer->setCurrentIndex( 0 );
                break;
            case 12:
                this->ui->box_StatsWarn_WebServer->setCurrentIndex(  1 );
                this->ui->box_StatsCount_WebServer->setCurrentIndex( 1 );
                this->ui->box_StatsSpeed_WebServer->setCurrentIndex( 1 );
                this->ui->box_StatsDay_WebServer->setCurrentIndex(   1 );
                this->ui->box_StatsRelat_WebServer->setCurrentIndex( 1 );
                break;
            case 13:
                this->ui->box_StatsWarn_WebServer->setCurrentIndex(  2 );
                this->ui->box_StatsCount_WebServer->setCurrentIndex( 2 );
                this->ui->box_StatsSpeed_WebServer->setCurrentIndex( 2 );
                this->ui->box_StatsDay_WebServer->setCurrentIndex(   2 );
                this->ui->box_StatsRelat_WebServer->setCurrentIndex( 2 );
                break;
            default:
                // shouldn't be here
                throw WebServerException( "Unexpected WebServer ID: "+std::to_string( this->default_ws ) );
        }
        this->initiating = false;
    }
}


const bool& MainWindow::checkDataDB()
{
    if ( this->initiating == false ) { // avoid recursions
        // check the db
        const std::string path = this->db_data_path + "/collection.db";
        bool ok = IOutils::checkFile( path, true );
        if ( ok == false ) {
            ok = CheckSec::checkStatsDatabase( path );
            // update ui stuff
            if ( ok == false ) {
                // checks failed
                this->crapview.clearDates();
                this->ui->box_StatsWarn_Year->clear();
                this->ui->box_StatsSpeed_Year->clear();
                this->ui->box_StatsCount_Year->clear();
                this->ui->box_StatsDay_FromYear->clear();
                if ( this->ui->checkBox_StatsDay_Period->isChecked() == true ) {
                    this->ui->box_StatsDay_ToYear->clear();
                }
                this->ui->box_StatsRelat_FromYear->clear();
                this->ui->box_StatsRelat_ToYear->clear();
            }
        }
        if ( ok == true && this->db_ok == false ) {
            this->db_ok = ok;
            this->initiating = true;
            this->refreshStatsDates();
            this->initiating = false;
        } else {
            this->db_ok = ok;
        }
    }
    return this->db_ok;
}


/////////////////////
//// GENERAL USE ////
/////////////////////
const std::string MainWindow::resolvePath( const std::string& path )
{
    std::string p;
    try {
        p = std::filesystem::canonical( StringOps::strip( path ) ).string();
    } catch (...) {
        ;
    }
    return p;
}
const std::string MainWindow::basePath( const std::string& path )
{
    const int stop = path.rfind( '/' );
    return path.substr( 0, stop );
}

// printable size with suffix and limited decimals
const QString MainWindow::printableSize( const int& bytes )
{
    std::string size_str, size_sfx=" B";
    float size = (float)bytes;
    if (size > 1024) {
        size /= 1024;
        size_sfx = " KiB";
        if (size > 1024) {
            size /= 1024;
            size_sfx = " MiB";
        }
    }
    // cut decimals depending on how big the floor is
    size_str = std::to_string( size );
    int cut_index = size_str.find('.')+1;
    if ( cut_index == 0 ) {
            cut_index = size_str.find(',')+1;
    }
    int n_decimals = 3;
    if ( size >= 100 ) {
        n_decimals = 2;
        if ( size >= 1000 ) {
            n_decimals = 1;
            if ( size >= 10000 ) {
                n_decimals = 0;
                cut_index --;
            }
        }
    }
    if ( cut_index >= 1 ) {
        cut_index += n_decimals;
        if ( cut_index > size_str.size()-1 ) {
            cut_index = size_str.size()-1;
        }
    }
    return QString::fromStdString( size_str.substr(0, cut_index ) + size_sfx );
}

// printable speed with suffix and limited decimals
const QString MainWindow::printableSpeed( const int& bytes, const int& secs_ )
{
    std::string speed_str, speed_sfx=" B/s";
    int secs = secs_;
    if ( secs == 0 ) {
        secs = 1;
    }
    float speed = (float)bytes / (float)secs;
    if (speed > 1024) {
        speed /= 1024;
        speed_sfx = " KiB/s";
        if (speed > 1024) {
            speed /= 1024;
            speed_sfx = " MiB/s";
        }
    }
    // cut decimals depending on how big the floor is
    speed_str = std::to_string( speed );
    int cut_index = speed_str.find('.')+1;
    if ( cut_index == 0 ) {
            cut_index = speed_str.find(',')+1;
    }
    int n_decimals = 3;
    if ( speed >= 100 ) {
        n_decimals = 2;
        if ( speed >= 1000 ) {
            n_decimals = 1;
            if ( speed >= 10000 ) {
                n_decimals = 0;
                cut_index --;
            }
        }
    }
    if ( cut_index >= 1 ) {
        cut_index += n_decimals;
        if ( cut_index > speed_str.size()-1 ) {
            cut_index = speed_str.size()-1;
        }
    }
    return QString::fromStdString( speed_str.substr(0, cut_index ) + speed_sfx );
}

const QString MainWindow::printableTime( const int& secs_ )
{
    int secs = secs_;
    int mins = secs / 60;
    secs = secs - (mins*60);
    std::string mins_str = (mins<10) ? "0"+std::to_string(mins) : std::to_string(mins);
    std::string secs_str = (secs<10) ? "0"+std::to_string(secs) : std::to_string(secs);
    return QString::fromStdString( mins_str +":"+ secs_str );
}


//////////////
//// HELP ////
//////////////
void MainWindow::showHelp( const std::string& filename )
{
    const std::string link = "https://github.com/elB4RTO/LogDoctor/tree/main/installation_stuff/logdocdata/help/";
    const std::string path =  this->logdoc_path+"/help/"+this->language+"/"+filename+".html";
    if ( IOutils::exists( path ) == true ) {
        if ( IOutils::isFile( path ) == true ) {
            if ( IOutils::checkFile( path, true ) == true ) {
                // everything ok, delete the old help window and open a new one
                delete this->craphelp;
                this->craphelp = new Craphelp();
                this->craphelp->helpLogsFormat(
                    path,
                    this->TB.getFont(),
                    this->TB.getColorSchemeID() );
                if ( this->isMaximized() == true ) {
                    this->craphelp->showMaximized();
                } else {
                    this->craphelp->show();
                }
            } else {
                // resource not readable
                DialogSec::errHelpNotReadable( nullptr, QString::fromStdString( link ) );
            }
        } else {
            // resource is not a file
            DialogSec::errHelpFailed( nullptr, QString::fromStdString( link ), QMessageBox::tr("unrecognized entry") );
        }
    } else {
        // resource not found
        DialogSec::errHelpNotFound( nullptr, QString::fromStdString( link ) );
    }
}



/***************************************************************
 * MainWindow'S OPERATIONS START FROM HERE
 ***************************************************************/


//////////////
//// MENU ////
/// //////////
// switch language
void MainWindow::menu_actionEnglish_triggered()
{
    this->ui->actionEnglish->setChecked( true );
    this->ui->actionEspanol->setChecked( false );
    this->ui->actionFrancais->setChecked( false );
    this->ui->actionItaliano->setChecked( false );
    this->language = "en";
    this->updateUiLanguage();
}
void MainWindow::menu_actionEspanol_triggered()
{
    this->ui->actionEnglish->setChecked( false );
    this->ui->actionEspanol->setChecked( true );
    this->ui->actionFrancais->setChecked( false );
    this->ui->actionItaliano->setChecked( false );
    this->language = "es";
    this->updateUiLanguage();
}
void MainWindow::menu_actionFrancais_triggered()
{
    this->ui->actionEnglish->setChecked( false );
    this->ui->actionEspanol->setChecked( false );
    this->ui->actionFrancais->setChecked( true );
    this->ui->actionItaliano->setChecked( false );
    this->language = "fr";
    this->updateUiLanguage();
}
void MainWindow::menu_actionItaliano_triggered()
{

    this->ui->actionEnglish->setChecked( false );
    this->ui->actionEspanol->setChecked( false );
    this->ui->actionFrancais->setChecked( false );
    this->ui->actionItaliano->setChecked( true );
    this->language = "it";
    this->updateUiLanguage();
}

// use a tool
void MainWindow::menu_actionBlockNote_triggered()
{
    if ( this->crapnote->isVisible() == true ) {
        this->crapnote->activateWindow();

    } else {
        delete this->crapnote;
        this->crapnote = new Crapnote();
        this->crapnote->setTextFont( this->TB.getFont() );
        this->crapnote->setColorScheme( this->TB.getColorSchemeID() );
        this->crapnote->show();
    }
}

void MainWindow::menu_actionCheckUpdates_triggered()
{
    this->crapup.versionCheck( this->version, this->dialogs_level );
}


//////////////
//// LOGS ////
//////////////
// switch to apache web server
void MainWindow::on_button_LogFiles_Apache_clicked()
{
    if ( this->craplog.getCurrentWSID() != 11 ) {
        // enable the enables
        this->ui->button_LogFiles_Apache->setFlat( false );
        this->ui->button_LogFiles_Nginx->setFlat( true );
        this->ui->button_LogFiles_Iis->setFlat( true );
        // load the list
        this->craplog.setCurrentWSID( 11 );
        this->on_button_LogFiles_RefreshList_clicked();
        QString rich_text;
        RichText::richLogsDefault( rich_text );
        this->ui->textLogFiles->setText( rich_text );
        rich_text.clear();
    }
}
// switch to nginx web server
void MainWindow::on_button_LogFiles_Nginx_clicked()
{
    if ( this->craplog.getCurrentWSID() != 12 ) {
        // enable the enables
        this->ui->button_LogFiles_Nginx->setFlat( false );
        this->ui->button_LogFiles_Apache->setFlat( true );
        this->ui->button_LogFiles_Iis->setFlat( true );
        // load the list
        this->craplog.setCurrentWSID( 12 );
        this->on_button_LogFiles_RefreshList_clicked();
        QString rich_text;
        RichText::richLogsDefault( rich_text );
        this->ui->textLogFiles->setText( rich_text );
        rich_text.clear();
    }
}
// switch to iis web server
void MainWindow::on_button_LogFiles_Iis_clicked()
{
    if ( this->craplog.getCurrentWSID() != 13 ) {
        // load the list
        this->ui->button_LogFiles_Iis->setFlat( false );
        this->ui->button_LogFiles_Apache->setFlat( true );
        this->ui->button_LogFiles_Nginx->setFlat( true );
        // load the list
        this->craplog.setCurrentWSID( 13 );
        this->on_button_LogFiles_RefreshList_clicked();
        QString rich_text;
        RichText::richLogsDefault( rich_text );
        this->ui->textLogFiles->setText( rich_text );
        rich_text.clear();
    }
}


// refresh the log files list
void MainWindow::on_button_LogFiles_RefreshList_clicked()
{
    std::string col;
    // clear the current tree
    this->ui->listLogFiles->clear();
    this->ui->checkBox_LogFiles_CheckAll->setCheckState( Qt::CheckState::Unchecked );
    // iterate over elements of list
    for ( const Craplog::LogFile& log_file : this->craplog.getLogsList(true) ) {
        // new entry for the tree widget
        QTreeWidgetItem *item = new QTreeWidgetItem();

        // preliminary check for file usage display
        if ( log_file.used_already == true ) {
            if ( this->hide_used_files == true ) {
                // do not display
                delete item; // possible memory leak
                continue;
            }
            // display with red foreground
            item->setForeground( 0, this->COLORS.at( "red" ) );
        }

        // preliminary check on file size
        col = "grey";
        if ( log_file.size > this->craplog.getWarningSize() ) {
            col = "orange";
        }
        item->setForeground( 1, this->COLORS.at( col ) );

        // set the name
        item->setText( 0, log_file.name );
        // set the size
        item->setText( 1, this->printableSize( log_file.size ) );
        item->setFont( 1, this->FONTS.at("main_italic") );
        // append the item (on top, forced)
        item->setCheckState(0, Qt::CheckState::Unchecked );
        this->ui->listLogFiles->addTopLevelItem( item );
    }
    if ( this->craplog.getLogsListSize() > 0 ) {
        // sort the list alphabetically
        this->ui->listLogFiles->sortByColumn(0, Qt::SortOrder::AscendingOrder );
        this->ui->checkBox_LogFiles_CheckAll->setEnabled( true );
    } else {
        this->ui->checkBox_LogFiles_CheckAll->setCheckState( Qt::CheckState::Unchecked );
        this->ui->checkBox_LogFiles_CheckAll->setEnabled( false );
    }
}


void MainWindow::on_checkBox_LogFiles_CheckAll_stateChanged(int arg1)
{
    Qt::CheckState new_state;
    if ( this->ui->checkBox_LogFiles_CheckAll->checkState() == Qt::CheckState::Checked ) {
        // check all
        new_state = Qt::CheckState::Checked;
        this->ui->button_MakeStats_Start->setEnabled(true);
    } else if ( this->ui->checkBox_LogFiles_CheckAll->checkState() == Qt::CheckState::Unchecked ) {
        // un-check all
        new_state = Qt::CheckState::Unchecked;
        this->ui->button_MakeStats_Start->setEnabled(false);
    } else {
        // do nothing
        this->ui->button_MakeStats_Start->setEnabled(true);
        return;
    }
    QTreeWidgetItemIterator i(this->ui->listLogFiles);
    while ( *i ) {
        (*i)->setCheckState( 0, new_state );
        ++i;
    }
}


void MainWindow::on_button_LogFiles_ViewFile_clicked()
{
    if ( this->ui->listLogFiles->selectedItems().size() > 0 ) {
        bool proceed = true;
        // display the selected item
        Craplog::LogFile item = this->craplog.getLogFileItem(
            this->ui->listLogFiles->selectedItems().takeFirst()->text(0) );
        FormatOps::LogsFormat format = this->craplog.getCurrentLogFormat();

        if ( proceed == true ) {
            std::string content;
            try {
                try {
                    // try reading as gzip compressed file
                    GZutils::readFile( item.path, content );

                } catch (GenericException& e) {
                    // failed closing file pointer
                    throw e;

                } catch (...) {
                    // failed as gzip, try as text file
                    if ( content.size() > 0 ) {
                        content.clear();
                    }
                    IOutils::readFile( item.path, content );
                }

            } catch (GenericException& e) {
                // failed closing
                proceed = false;
                // >> err.what() << //
                DialogSec::errGeneric( nullptr, QMessageBox::tr("Failed closing file pointer for:\n") + item.name );

            } catch (const std::ios_base::failure& err) {
                // failed reading
                proceed = false;
                // >> err.what() << //
                DialogSec::errFailedReadFile( nullptr, item.name );
            } catch (...) {
                // failed somehow
                proceed = false;
                DialogSec::errFailedReadFile( nullptr, item.name );
            }

            if ( proceed == true ) {
                // succesfully read, now enriched and display
                QString rich_content;
                RichText::enrichLogs(
                    rich_content, content,
                    format, this->TB );
                this->ui->textLogFiles->setText( rich_content );
                this->ui->textLogFiles->setFont( this->TB.getFont() );
                rich_content.clear();
            }
            content.clear();
        }
        if ( proceed == false ) {
            // failed
            QString rich_text;
            RichText::richLogsFailure( rich_text );
            this->ui->textLogFiles->setText( rich_text );
            rich_text.clear();
        }
    }
}


void MainWindow::on_listLogFiles_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    this->on_button_LogFiles_ViewFile_clicked();
}


void MainWindow::on_listLogFiles_itemChanged(QTreeWidgetItem *item, int column)
{
    // control checked
    int n_checked = 0;
    QTreeWidgetItemIterator i(this->ui->listLogFiles);
    while ( *i ) {
        if ( (*i)->checkState(0) == Qt::CheckState::Checked ) {
            n_checked++;
        }
        ++i;
    }
    if ( n_checked == 0 ) {
        this->ui->checkBox_LogFiles_CheckAll->setCheckState(Qt::CheckState::Unchecked);
    } else if ( n_checked == this->craplog.getLogsListSize() ) {
        this->ui->checkBox_LogFiles_CheckAll->setCheckState(Qt::CheckState::Checked);
    } else {
        this->ui->checkBox_LogFiles_CheckAll->setCheckState(Qt::CheckState::PartiallyChecked);
    }
}


void MainWindow::on_button_MakeStats_Start_clicked()
{
    bool proceed = true;
    // check that the format has been set
    const FormatOps::LogsFormat& lf = this->craplog.getLogsFormat( this->craplog.getCurrentWSID() );
    if ( lf.string.size() == 0 ) {
        // format string not set
        proceed = false;
        DialogSec::errLogFormatNotSet( nullptr );
    } else if ( lf.fields.size() == 0 ) {
        // no field, useless to parse
        proceed = false;
        DialogSec::errLogFormatNoFields( nullptr );
    } else if ( lf.separators.size() < lf.fields.size()-2 ) {
        // missing at least a separator between two (or more) fields
        proceed = false;
        DialogSec::errLogFormatNoSeparators( nullptr );
    }

    if ( proceed == true ) {
        // take actions on Craplog's start
        this->craplogStarted();

        // feed craplog with the checked files
        QTreeWidgetItemIterator i(this->ui->listLogFiles);
        while ( *i ) {
            if ( (*i)->checkState(0) == Qt::CheckState::Checked ) {
                // tell Craplog to set this file as selected
                if ( this->craplog.setLogFileSelected( (*i)->text(0) ) == false ) {
                    // this shouldn't be, but...
                    if ( DialogSec::choiceSelectedFileNotFound( nullptr, (*i)->text(0) ) == false ) {
                        proceed = false;
                        break;
                    }
                }
            }
            ++i;
        }

        if ( proceed == true ) {
            // check files to be used before to start
            proceed = this->craplog.checkStuff();
        } else {
            this->craplogFinished();
        }

        if ( proceed == true ) {
            // periodically update perfs
            this->craplog_timer = new QTimer(this);
            connect(this->craplog_timer, SIGNAL(timeout()), this, SLOT(update_Craplog_PerfData()));
            this->craplog_timer->start(250);
            // run craplog as thread
            this->craplog_timer_start = std::chrono::system_clock::now();
            this->craplog_thread = std::thread( &Craplog::run, &this->craplog );
        } else {
            this->craplogFinished();
        }
    }
}

void MainWindow::reset_MakeStats_labels()
{
    // reset to default
    this->ui->label_MakeStats_Size->setText( "0 B" );
    this->ui->label_MakeStats_Lines->setText( "0" );
    // time and speed
    this->ui->label_MakeStats_Time->setText( "00:00" );
    this->ui->label_MakeStats_Speed->setText( "0 B/s" );
}

void MainWindow::update_MakeStats_labels()
{
    // update values
    int size, secs;
    // size and lines
    if ( this->craplog.isParsing() == true ) {
        this->craplog.collectPerfData();
    }
    size = this->craplog.getTotalSize();
    //size = this->craplog.getParsedSize();
    this->ui->label_MakeStats_Size->setText( this->printableSize( size ) );
    this->ui->label_MakeStats_Lines->setText( QString::fromStdString(std::to_string(this->craplog.getParsedLines())) );
    // time and speed
    this->craplog_timer_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        this->craplog_timer_start - std::chrono::system_clock::now()
    );
    size = this->craplog.getPerfSize();
    secs = this->craplog_timer_elapsed.count() / -1000000000;
    this->ui->label_MakeStats_Time->setText( this->printableTime( secs ));
    this->ui->label_MakeStats_Speed->setText( this->printableSpeed( size, secs ));
}

void MainWindow::update_Craplog_PerfData()
{
    // craplog is running as thread, update the values meanwhile
    this->update_MakeStats_labels();
    // check if Craplog has finished working
    if ( this->craplog.isWorking() == false ) {
        this->craplog_timer->stop();
        this->craplog_thread.join();
        this->craplogFinished();
    }
}

void MainWindow::craplogStarted()
{
    // reset perfs
    this->reset_MakeStats_labels();
    this->craplog.logOps.resetPerfData();
    // disable the LogFiles section
    this->ui->LogBoxFiles->setEnabled(false);
    // disable the start button
    this->ui->button_MakeStats_Start->setEnabled(false);
    // enable all labels (needed only the first time)
    this->ui->icon_MakeStats_Size->setEnabled(false);
    this->ui->label_MakeStats_Size->setEnabled(true);
    this->ui->icon_MakeStats_Lines->setEnabled(false);
    this->ui->label_MakeStats_Lines->setEnabled(true);
    this->ui->icon_MakeStats_Time->setEnabled(false);
    this->ui->label_MakeStats_Time->setEnabled(true);
    this->ui->icon_MakeStats_Speed->setEnabled(false);
    this->ui->label_MakeStats_Speed->setEnabled(true);
    // disable the stats/settings tab
    this->ui->View->setEnabled(false);
    this->ui->Set->setEnabled(false);
}

void MainWindow::craplogFinished()
{
    // update the perf data one last time, just in case
    this->update_MakeStats_labels();
    this->craplog.makeCharts(
        this->CHARTS_THEMES.at( this->charts_theme_id ), this->FONTS,
        this->ui->chart_MakeStats_Size );
    // clean up temp vars
    this->craplog.clearDataCollection();
    this->craplog.logOps.resetPerfData();

    // refresh the logs list
    this->on_button_LogFiles_RefreshList_clicked();
    // enable the LogFiles section
    this->ui->LogBoxFiles->setEnabled(true);
    // enable all labels (needed only the first time each session)
    this->ui->icon_MakeStats_Size->setEnabled(true);
    this->ui->icon_MakeStats_Lines->setEnabled(true);
    this->ui->icon_MakeStats_Time->setEnabled(true);
    this->ui->icon_MakeStats_Speed->setEnabled(true);
    // enable back the stats/settings tab
    this->ui->View->setEnabled(true);
    this->ui->Set->setEnabled(true);
    // get a fresh collection of available stats dates
    this->refreshStatsDates();
}



///////////////
//// STATS ////
///////////////
// refresh all the dates boxes
void MainWindow::refreshStatsDates()
{
    this->crapview.refreshDates();
    this->on_box_StatsWarn_WebServer_currentIndexChanged(  this->ui->box_StatsWarn_WebServer->currentIndex()  );
    this->on_box_StatsSpeed_WebServer_currentIndexChanged( this->ui->box_StatsSpeed_WebServer->currentIndex() );
    this->on_box_StatsCount_WebServer_currentIndexChanged( this->ui->box_StatsCount_WebServer->currentIndex() );
    this->on_box_StatsDay_WebServer_currentIndexChanged(   this->ui->box_StatsDay_WebServer->currentIndex()   );
    this->on_box_StatsRelat_WebServer_currentIndexChanged( this->ui->box_StatsRelat_WebServer->currentIndex() );
}


//////////////
//// WARN ////
void MainWindow::checkStatsWarnDrawable()
{
    if ( this->ui->box_StatsWarn_Year->currentIndex() >= 0
      && this->ui->box_StatsWarn_Month->currentIndex() >= 0
      && this->ui->box_StatsWarn_Day->currentIndex() >= 0 ) {
        // enable the draw button
        this->ui->button_StatsWarn_Draw->setEnabled( true );
    } else {
        // disable the draw button
        this->ui->button_StatsWarn_Draw->setEnabled( false );
    }
}

void MainWindow::on_box_StatsWarn_WebServer_currentIndexChanged(int index)
{
    if ( this->checkDataDB() == true ) {
        this->ui->box_StatsWarn_Year->clear();
        if ( index != -1 ) {
            this->ui->box_StatsWarn_Year->addItems(
                this->crapview.getYears(
                    this->ui->box_StatsWarn_WebServer->currentText() ));
            this->ui->box_StatsWarn_Year->setCurrentIndex( 0 );
        }
    }
    this->checkStatsWarnDrawable();
}

void MainWindow::on_box_StatsWarn_Year_currentIndexChanged(int index)
{
    this->ui->box_StatsWarn_Month->clear();
    if ( index != -1 ) {
        this->ui->box_StatsWarn_Month->addItems(
            this->crapview.getMonths(
                this->ui->box_StatsWarn_WebServer->currentText(),
                this->ui->box_StatsWarn_Year->currentText() ) );
        this->ui->box_StatsWarn_Month->setCurrentIndex( 0 );
    }
    this->checkStatsWarnDrawable();
}

void MainWindow::on_box_StatsWarn_Month_currentIndexChanged(int index)
{
    this->ui->box_StatsWarn_Day->clear();
    if ( index != -1 ) {
        this->ui->box_StatsWarn_Day->addItems(
            this->crapview.getDays(
                this->ui->box_StatsWarn_WebServer->currentText(),
                this->ui->box_StatsWarn_Year->currentText(),
                this->ui->box_StatsWarn_Month->currentText() ) );
        this->ui->box_StatsWarn_Day->setCurrentIndex( 0 );
    }
    this->checkStatsWarnDrawable();
}

void MainWindow::on_box_StatsWarn_Day_currentIndexChanged(int index)
{
    if ( this->ui->checkBox_StatsWarn_Hour->isChecked() == true ) {
        this->ui->box_StatsWarn_Hour->clear();
        if ( index != -1 ) {
            this->ui->box_StatsWarn_Hour->addItems( this->crapview.getHours() );
            this->ui->box_StatsWarn_Hour->setCurrentIndex( 0 );
        }
    }
    this->checkStatsWarnDrawable();
}

void MainWindow::on_checkBox_StatsWarn_Hour_stateChanged(int state)
{
    if ( state == Qt::CheckState::Checked ) {
        this->ui->box_StatsWarn_Hour->setEnabled( true );
        // add available dates
        this->on_box_StatsWarn_Day_currentIndexChanged( 0 );
    } else {
        this->ui->box_StatsWarn_Hour->clear();
        this->ui->box_StatsWarn_Hour->setEnabled( false );
    }
}

void MainWindow::on_box_StatsWarn_Hour_currentIndexChanged(int index)
{
    this->checkStatsWarnDrawable();
}

void MainWindow::on_button_StatsWarn_Draw_clicked()
{
    if ( this->checkDataDB() == true ) {
        this->ui->table_StatsWarn->setRowCount(0);
        this->crapview.drawWarn(
            this->ui->table_StatsWarn, this->ui->chart_StatsWarn,
            this->CHARTS_THEMES.at( this->charts_theme_id ), this->FONTS,
            this->ui->box_StatsWarn_WebServer->currentText(),
            this->ui->box_StatsWarn_Year->currentText(),
            this->ui->box_StatsWarn_Month->currentText(),
            this->ui->box_StatsWarn_Day->currentText(),
            (this->ui->checkBox_StatsWarn_Hour->isChecked()) ? this->ui->box_StatsWarn_Hour->currentText() : "" );
        this->ui->button_StatsWarn_Update->setEnabled( true );
    }
}


void MainWindow::on_button_StatsWarn_Update_clicked()
{
    this->crapview.updateWarn(
        this->ui->table_StatsWarn,
        this->ui->box_StatsWarn_WebServer->currentText() );
}


///////////////
//// SPEED ////
void MainWindow::checkStatsSpeedDrawable()
{
    if ( this->ui->box_StatsSpeed_Year->currentIndex() >= 0
      && this->ui->box_StatsSpeed_Month->currentIndex() >= 0
      && this->ui->box_StatsSpeed_Day->currentIndex() >= 0 ) {
        // enable the draw button
        this->ui->button_StatsSpeed_Draw->setEnabled( true );
    } else {
        // disable the draw button
        this->ui->button_StatsSpeed_Draw->setEnabled( false );
    }
}

void MainWindow::on_box_StatsSpeed_WebServer_currentIndexChanged(int index)
{
    if ( this->checkDataDB() == true ) {
        this->ui->box_StatsSpeed_Year->clear();
        if ( index != -1 ) {
            this->ui->box_StatsSpeed_Year->addItems(
                this->crapview.getYears(
                    this->ui->box_StatsSpeed_WebServer->currentText() ) );
            this->ui->box_StatsSpeed_Year->setCurrentIndex( 0 );
        }
    }
    this->checkStatsSpeedDrawable();
}

void MainWindow::on_box_StatsSpeed_Year_currentIndexChanged(int index)
{
    this->ui->box_StatsSpeed_Month->clear();
    if ( index != -1 ) {
        this->ui->box_StatsSpeed_Month->addItems(
            this->crapview.getMonths(
                this->ui->box_StatsSpeed_WebServer->currentText(),
                this->ui->box_StatsSpeed_Year->currentText() ) );
        this->ui->box_StatsSpeed_Month->setCurrentIndex( 0 );
    }
    this->checkStatsSpeedDrawable();
}

void MainWindow::on_box_StatsSpeed_Month_currentIndexChanged(int index)
{
    this->ui->box_StatsSpeed_Day->clear();
    if ( index != -1 ) {
        this->ui->box_StatsSpeed_Day->addItems(
            this->crapview.getDays(
                this->ui->box_StatsSpeed_WebServer->currentText(),
                this->ui->box_StatsSpeed_Year->currentText(),
                this->ui->box_StatsSpeed_Month->currentText() ) );
        this->ui->box_StatsSpeed_Day->setCurrentIndex( 0 );
    }
    this->checkStatsSpeedDrawable();
}

void MainWindow::on_box_StatsSpeed_Day_currentIndexChanged(int index)
{
    this->checkStatsSpeedDrawable();
}

void MainWindow::on_button_StatsSpeed_Draw_clicked()
{
    if ( this->checkDataDB() == true ) {
        this->ui->table_StatsSpeed->setRowCount(0);
        this->crapview.drawSpeed(
            this->ui->table_StatsSpeed,
            this->ui->chart_SatsSpeed,
            this->CHARTS_THEMES.at( this->charts_theme_id ), this->FONTS,
            this->ui->box_StatsSpeed_WebServer->currentText(),
            this->ui->box_StatsSpeed_Year->currentText(),
            this->ui->box_StatsSpeed_Month->currentText(),
            this->ui->box_StatsSpeed_Day->currentText(),
            this->crapview.parseTextualFilter( this->ui->inLine_StatsSpeed_Protocol->text() ),
            this->crapview.parseTextualFilter( this->ui->inLine_StatsSpeed_Method->text() ),
            this->crapview.parseTextualFilter( this->ui->inLine_StatsSpeed_Uri->text() ),
            this->crapview.parseTextualFilter( this->ui->inLine_StatsSpeed_Query->text() ),
            this->crapview.parseNumericFilter( this->ui->inLine_StatsSpeed_Response->text() ) );
    }
}


///////////////
//// COUNT ////
void MainWindow::checkStatsCountDrawable()
{
    if ( this->ui->box_StatsCount_Year->currentIndex() >= 0
      && this->ui->box_StatsCount_Month->currentIndex() >= 0
      && this->ui->box_StatsCount_Day->currentIndex() >= 0 ) {
        // enable the draw button
        this->ui->scrollArea_StatsCount->setEnabled( true );
    } else {
        // disable the draw button
        this->ui->scrollArea_StatsCount->setEnabled( false );
    }
}

void MainWindow::on_box_StatsCount_WebServer_currentIndexChanged(int index)
{
    if ( this->checkDataDB() == true ) {
        this->ui->box_StatsCount_Year->clear();
        if ( index != -1 ) {
            this->ui->box_StatsCount_Year->addItems(
                this->crapview.getYears(
                    this->ui->box_StatsCount_WebServer->currentText() ));
            this->ui->box_StatsCount_Year->setCurrentIndex( 0 );
            this->resetStatsCountButtons();
        }
    }
    this->checkStatsCountDrawable();
}

void MainWindow::on_box_StatsCount_Year_currentIndexChanged(int index)
{
    this->ui->box_StatsCount_Month->clear();
    if ( index != -1 ) {
        this->ui->box_StatsCount_Month->addItems(
            this->crapview.getMonths(
                this->ui->box_StatsCount_WebServer->currentText(),
                this->ui->box_StatsCount_Year->currentText() ) );
        this->ui->box_StatsCount_Month->setCurrentIndex( 0 );
    }
    this->checkStatsCountDrawable();
}

void MainWindow::on_box_StatsCount_Month_currentIndexChanged(int index)
{
    this->ui->box_StatsCount_Day->clear();
    if ( index != -1 ) {
        this->ui->box_StatsCount_Day->addItems(
            this->crapview.getDays(
                this->ui->box_StatsCount_WebServer->currentText(),
                this->ui->box_StatsCount_Year->currentText(),
                this->ui->box_StatsCount_Month->currentText() ) );
        this->ui->box_StatsCount_Day->setCurrentIndex( 0 );
    }
    this->checkStatsCountDrawable();
}

void MainWindow::on_box_StatsCount_Day_currentIndexChanged(int index)
{
    this->checkStatsCountDrawable();
}

void MainWindow::resetStatsCountButtons()
{
    if ( this->ui->button_StatsCount_Protocol->isFlat() == false ) {
        this->ui->button_StatsCount_Protocol->setFlat( true );
    }
    if ( this->ui->button_StatsCount_Method->isFlat() == false ) {
        this->ui->button_StatsCount_Method->setFlat( true );
    }
    if ( this->ui->button_StatsCount_Request->isFlat() == false ) {
        this->ui->button_StatsCount_Request->setFlat( true );
    }
    if ( this->ui->button_StatsCount_Query->isFlat() == false ) {
        this->ui->button_StatsCount_Query->setFlat( true );
    }
    if ( this->ui->button_StatsCount_Response->isFlat() == false ) {
        this->ui->button_StatsCount_Response->setFlat( true );
    }
    if ( this->ui->button_StatsCount_Referrer->isFlat() == false ) {
        this->ui->button_StatsCount_Referrer->setFlat( true );
    }
    if ( this->ui->button_StatsCount_Cookie->isFlat() == false ) {
        this->ui->button_StatsCount_Cookie->setFlat( true );
    }
    if ( this->ui->button_StatsCount_UserAgent->isFlat() == false ) {
        this->ui->button_StatsCount_UserAgent->setFlat( true );
    }
    if ( this->ui->button_StatsCount_Client->isFlat() == false ) {
        this->ui->button_StatsCount_Client->setFlat( true );
    }
}

void MainWindow::on_button_StatsCount_Protocol_clicked()
{
    this->drawStatsCount( this->ui->button_StatsCount_Protocol->text() );
    this->ui->button_StatsCount_Protocol->setFlat( false );
}

void MainWindow::on_button_StatsCount_Method_clicked()
{
    this->drawStatsCount( this->ui->button_StatsCount_Method->text() );
    this->ui->button_StatsCount_Method->setFlat( false );
}

void MainWindow::on_button_StatsCount_Request_clicked()
{
    this->drawStatsCount( this->ui->button_StatsCount_Request->text() );
    this->ui->button_StatsCount_Request->setFlat( false );
}

void MainWindow::on_button_StatsCount_Query_clicked()
{
    this->drawStatsCount( this->ui->button_StatsCount_Query->text() );
    this->ui->button_StatsCount_Query->setFlat( false );
}

void MainWindow::on_button_StatsCount_Response_clicked()
{
    this->drawStatsCount( this->ui->button_StatsCount_Response->text() );
    this->ui->button_StatsCount_Response->setFlat( false );
}

void MainWindow::on_button_StatsCount_Referrer_clicked()
{
    this->drawStatsCount( this->ui->button_StatsCount_Referrer->text() );
    this->ui->button_StatsCount_Referrer->setFlat( false );
}

void MainWindow::on_button_StatsCount_Cookie_clicked()
{
    this->drawStatsCount( this->ui->button_StatsCount_Cookie->text() );
    this->ui->button_StatsCount_Cookie->setFlat( false );
}

void MainWindow::on_button_StatsCount_UserAgent_clicked()
{
    this->drawStatsCount( this->ui->button_StatsCount_UserAgent->text() );
    this->ui->button_StatsCount_UserAgent->setFlat( false );
}

void MainWindow::on_button_StatsCount_Client_clicked()
{
    this->drawStatsCount( this->ui->button_StatsCount_Client->text() );
    this->ui->button_StatsCount_Client->setFlat( false );
}

void MainWindow::drawStatsCount( const QString& field )
{
    this->ui->table_StatsCount->setRowCount(0);
    this->crapview.drawCount(
        this->ui->table_StatsCount, this->ui->chart_StatsCount,
        this->CHARTS_THEMES.at( this->charts_theme_id ), this->FONTS,
        this->ui->box_StatsCount_WebServer->currentText(),
        this->ui->box_StatsCount_Year->currentText(), this->ui->box_StatsCount_Month->currentText(), this->ui->box_StatsCount_Day->currentText(),
        field );
    this->resetStatsCountButtons();
}


/////////////
//// DAY ////
void MainWindow::checkStatsDayDrawable()
{
    bool aux = true;
    // secondary date (period)
    if ( this->ui->checkBox_StatsDay_Period->isChecked() == true ) {
        if ( this->ui->box_StatsDay_ToYear->currentIndex() < 0
          && this->ui->box_StatsDay_ToMonth->currentIndex() < 0
          && this->ui->box_StatsDay_ToDay->currentIndex() < 0 ) {
           aux = false;
        } else {
            int a,b;
            a = this->ui->box_StatsDay_ToYear->currentText().toInt();
            b = this->ui->box_StatsDay_FromYear->currentText().toInt();
            if ( a < b ) {
                // year 'to' is less than 'from'
                aux = false;
            } else if ( a == b ) {
                a = this->crapview.getMonthNumber( this->ui->box_StatsDay_ToMonth->currentText() );
                b = this->crapview.getMonthNumber( this->ui->box_StatsDay_FromMonth->currentText() );
                if ( a < b ) {
                    // month 'to' is less than 'from'
                    aux = false;
                } else if ( a == b ) {
                    a = this->ui->box_StatsDay_ToDay->currentText().toInt();
                    b = this->ui->box_StatsDay_FromDay->currentText().toInt();
                    if ( a < b ) {
                        // day 'to' is less than 'from'
                        aux = false;
                    }
                }
            }
        }
    }
    // primary date
    if ( this->ui->box_StatsDay_FromYear->currentIndex() < 0
      && this->ui->box_StatsDay_FromMonth->currentIndex() < 0
      && this->ui->box_StatsDay_FromDay->currentIndex() < 0 ) {
        aux = false;
    }
    // check log field validity
    if ( this->ui->box_StatsDay_LogsField->currentIndex() < 0 ) {
        aux = false;
    }
    this->ui->button_StatsDay_Draw->setEnabled( aux);
}

void MainWindow::on_box_StatsDay_WebServer_currentIndexChanged(int index)
{
    if ( this->checkDataDB() == true ) {
        this->ui->box_StatsDay_LogsField->clear();
        this->ui->box_StatsDay_FromYear->clear();
        this->ui->box_StatsDay_ToYear->clear();
        if ( index != -1 ) {
            // refresh fields
            this->ui->box_StatsDay_LogsField->addItems(
                this->crapview.getFields( "Daytime" ));
            this->ui->box_StatsDay_LogsField->setCurrentIndex( 0 );
            // refresh dates
            QStringList years = this->crapview.getYears(
                this->ui->box_StatsDay_WebServer->currentText() );
            this->ui->box_StatsDay_FromYear->addItems( years );
            this->ui->box_StatsDay_FromYear->setCurrentIndex( 0 );
            if ( this->ui->checkBox_StatsDay_Period->isChecked() == true ) {
                this->ui->box_StatsDay_ToYear->addItems( years );
                this->ui->box_StatsDay_ToYear->setCurrentIndex( 0 );
            }
            years.clear();
        }
    }
    this->checkStatsDayDrawable();
}


void MainWindow::on_box_StatsDay_LogsField_currentIndexChanged(int index)
{
    this->ui->inLine_StatsDay_Filter->clear();
    this->checkStatsDayDrawable();
}

void MainWindow::on_box_StatsDay_FromYear_currentIndexChanged(int index)
{
    this->ui->box_StatsDay_FromMonth->clear();
    if ( index != -1 ) {
        this->ui->box_StatsDay_FromMonth->addItems(
            this->crapview.getMonths(
                this->ui->box_StatsDay_WebServer->currentText(),
                this->ui->box_StatsDay_FromYear->currentText() ) );
        this->ui->box_StatsDay_FromMonth->setCurrentIndex( 0 );
    }
    this->checkStatsDayDrawable();
}

void MainWindow::on_box_StatsDay_FromMonth_currentIndexChanged(int index)
{
    this->ui->box_StatsDay_FromDay->clear();
    if ( index != -1 ) {
        this->ui->box_StatsDay_FromDay->addItems(
            this->crapview.getDays(
                this->ui->box_StatsDay_WebServer->currentText(),
                this->ui->box_StatsDay_FromYear->currentText(),
                this->ui->box_StatsDay_FromMonth->currentText() ) );
        this->ui->box_StatsDay_FromDay->setCurrentIndex( 0 );
    }
    this->checkStatsDayDrawable();
}

void MainWindow::on_box_StatsDay_FromDay_currentIndexChanged(int index)
{
    this->checkStatsDayDrawable();
}

void MainWindow::on_checkBox_StatsDay_Period_stateChanged(int state)
{
    if ( state == Qt::CheckState::Checked ) {
        this->ui->box_StatsDay_ToYear->setEnabled( true );
        this->ui->box_StatsDay_ToMonth->setEnabled( true );
        this->ui->box_StatsDay_ToDay->setEnabled( true );
        // add available dates
        this->ui->box_StatsDay_ToYear->addItems( this->crapview.getYears(
            this->ui->box_StatsDay_WebServer->currentText() ) );
        this->ui->box_StatsDay_ToYear->setCurrentIndex( 0 );
    } else {
        this->ui->box_StatsDay_ToYear->clear();
        this->ui->box_StatsDay_ToYear->setEnabled( false );
        this->ui->box_StatsDay_ToMonth->clear();
        this->ui->box_StatsDay_ToMonth->setEnabled( false );
        this->ui->box_StatsDay_ToDay->clear();
        this->ui->box_StatsDay_ToDay->setEnabled( false );
    }
}

void MainWindow::on_box_StatsDay_ToYear_currentIndexChanged(int index)
{
    this->ui->box_StatsDay_ToMonth->clear();
    if ( index != -1 ) {
        this->ui->box_StatsDay_ToMonth->addItems(
            this->crapview.getMonths(
                this->ui->box_StatsDay_WebServer->currentText(),
                this->ui->box_StatsDay_ToYear->currentText() ) );
        this->ui->box_StatsDay_ToMonth->setCurrentIndex( 0 );
    }
    this->checkStatsDayDrawable();
}

void MainWindow::on_box_StatsDay_ToMonth_currentIndexChanged(int index)
{
    this->ui->box_StatsDay_ToDay->clear();
    if ( index != -1 ) {
        this->ui->box_StatsDay_ToDay->addItems(
            this->crapview.getDays(
                this->ui->box_StatsDay_WebServer->currentText(),
                this->ui->box_StatsDay_ToYear->currentText(),
                this->ui->box_StatsDay_ToMonth->currentText() ) );
        this->ui->box_StatsDay_ToDay->setCurrentIndex( 0 );
    }
    this->checkStatsDayDrawable();
}

void MainWindow::on_box_StatsDay_ToDay_currentIndexChanged(int index)
{
    this->checkStatsDayDrawable();
}


void MainWindow::on_button_StatsDay_Draw_clicked()
{
    if ( this->checkDataDB() == true ) {
        QString filter;
        if ( this->ui->box_StatsDay_LogsField->currentIndex() == 0 ) {
            filter = this->crapview.parseBooleanFilter( this->ui->inLine_StatsDay_Filter->text() );
        } else if ( this->ui->box_StatsDay_LogsField->currentIndex() == 5 ) {
            filter = this->crapview.parseNumericFilter( this->ui->inLine_StatsDay_Filter->text() );
        } else {
            filter = this->crapview.parseTextualFilter( this->ui->inLine_StatsDay_Filter->text() );
        }
        this->crapview.drawDay(
            this->ui->chart_StatsDay,
            this->CHARTS_THEMES.at( this->charts_theme_id ), this->FONTS,
            this->ui->box_StatsDay_WebServer->currentText(),
            this->ui->box_StatsDay_FromYear->currentText(),
            this->ui->box_StatsDay_FromMonth->currentText(),
            this->ui->box_StatsDay_FromDay->currentText(),
            ( this->ui->checkBox_StatsDay_Period->isChecked() ) ? this->ui->box_StatsDay_ToYear->currentText() : "",
            ( this->ui->checkBox_StatsDay_Period->isChecked() ) ? this->ui->box_StatsDay_ToMonth->currentText() : "",
            ( this->ui->checkBox_StatsDay_Period->isChecked() ) ? this->ui->box_StatsDay_ToDay->currentText() : "",
            this->ui->box_StatsDay_LogsField->currentText(),
            filter );
    }
}



////////////////////
//// RELATIONAL ////
void MainWindow::checkStatsRelatDrawable()
{
    bool aux = true;
    if ( this->ui->box_StatsRelat_FromYear->currentIndex() >= 0
      && this->ui->box_StatsRelat_FromMonth->currentIndex() >= 0
      && this->ui->box_StatsRelat_FromDay->currentIndex() >= 0
      && this->ui->box_StatsRelat_ToYear->currentIndex() >= 0
      && this->ui->box_StatsRelat_ToMonth->currentIndex() >= 0
      && this->ui->box_StatsRelat_ToDay->currentIndex() >= 0 ) {
        // check period validity
        int a,b;
        a = this->ui->box_StatsRelat_ToYear->currentText().toInt();
        b = this->ui->box_StatsRelat_FromYear->currentText().toInt();
        if ( a < b ) {
            // year 'to' is less than 'from'
            aux = false;
        } else if ( a == b ) {
            a = this->crapview.getMonthNumber( this->ui->box_StatsRelat_ToMonth->currentText() );
            b = this->crapview.getMonthNumber( this->ui->box_StatsRelat_FromMonth->currentText() );
            if ( a < b ) {
                // month 'to' is less than 'from'
                aux = false;
            } else if ( a == b ) {
                a = this->ui->box_StatsRelat_ToDay->currentText().toInt();
                b = this->ui->box_StatsRelat_FromDay->currentText().toInt();
                if ( a < b ) {
                    // day 'to' is less than 'from'
                    aux = false;
                }
            }
        }
    } else {
        // disable the draw button
        aux = false;
    }
    // check log field validity
    if ( this->ui->box_StatsRelat_LogsField_1->currentIndex() < 0
      || this->ui->box_StatsRelat_LogsField_2->currentIndex() < 0 ) {
        aux = false;
    }
    this->ui->button_StatsRelat_Draw->setEnabled( aux );
}

void MainWindow::on_box_StatsRelat_WebServer_currentIndexChanged(int index)
{
    if ( this->checkDataDB() == true ) {
        this->ui->box_StatsRelat_LogsField_1->clear();
        this->ui->box_StatsRelat_LogsField_2->clear();
        if ( index != -1 ) {
            // refresh fields
            QStringList fields = this->crapview.getFields( "Relational" );
            this->ui->box_StatsRelat_LogsField_1->addItems( fields );
            this->ui->box_StatsRelat_LogsField_2->addItems( fields );
            this->ui->box_StatsRelat_LogsField_1->setCurrentIndex( 0 );
            this->ui->box_StatsRelat_LogsField_2->setCurrentIndex( 0 );
            // refresh dates
            QStringList years = this->crapview.getYears(
                this->ui->box_StatsRelat_WebServer->currentText() );
            // from
            this->ui->box_StatsRelat_FromYear->clear();
            this->ui->box_StatsRelat_FromYear->addItems( years );
            this->ui->box_StatsRelat_FromYear->setCurrentIndex( 0 );
            // to
            this->ui->box_StatsRelat_ToYear->clear();
            this->ui->box_StatsRelat_ToYear->addItems( years );
            this->ui->box_StatsRelat_ToYear->setCurrentIndex( 0 );
            years.clear();
        }
    }
    this->checkStatsRelatDrawable();
}

void MainWindow::on_box_StatsRelat_LogsField_1_currentIndexChanged(int index)
{
    this->ui->inLine_StatsRelat_Filter_1->clear();
    this->checkStatsRelatDrawable();
}

void MainWindow::on_box_StatsRelat_LogsField_2_currentIndexChanged(int index)
{
    this->ui->inLine_StatsRelat_Filter_2->clear();
    this->checkStatsRelatDrawable();
}

void MainWindow::on_box_StatsRelat_FromYear_currentIndexChanged(int index)
{
    this->ui->box_StatsRelat_FromMonth->clear();
    if ( index != -1 ) {
        this->ui->box_StatsRelat_FromMonth->addItems(
            this->crapview.getMonths(
                this->ui->box_StatsRelat_WebServer->currentText(),
                this->ui->box_StatsRelat_FromYear->currentText() ) );
        this->ui->box_StatsRelat_FromMonth->setCurrentIndex( 0 );
    }
    this->checkStatsRelatDrawable();
}

void MainWindow::on_box_StatsRelat_FromMonth_currentIndexChanged(int index)
{
    this->ui->box_StatsRelat_FromDay->clear();
    if ( index != -1 ) {
        this->ui->box_StatsRelat_FromDay->addItems(
            this->crapview.getDays(
                this->ui->box_StatsRelat_WebServer->currentText(),
                this->ui->box_StatsRelat_FromYear->currentText(),
                this->ui->box_StatsRelat_FromMonth->currentText() ) );
        this->ui->box_StatsRelat_FromDay->setCurrentIndex( 0 );
    }
    this->checkStatsRelatDrawable();
}

void MainWindow::on_box_StatsRelat_FromDay_currentIndexChanged(int index)
{
    this->checkStatsRelatDrawable();
}

void MainWindow::on_box_StatsRelat_ToYear_currentIndexChanged(int index)
{
    this->ui->box_StatsRelat_ToMonth->clear();
    if ( index != -1 ) {
        this->ui->box_StatsRelat_ToMonth->addItems(
            this->crapview.getMonths(
                this->ui->box_StatsRelat_WebServer->currentText(),
                this->ui->box_StatsRelat_ToYear->currentText() ) );
        this->ui->box_StatsRelat_ToMonth->setCurrentIndex( 0 );
    }
    this->checkStatsRelatDrawable();
}

void MainWindow::on_box_StatsRelat_ToMonth_currentIndexChanged(int index)
{
    this->ui->box_StatsRelat_ToDay->clear();
    if ( index != -1 ) {
        this->ui->box_StatsRelat_ToDay->addItems(
            this->crapview.getDays(
                this->ui->box_StatsRelat_WebServer->currentText(),
                this->ui->box_StatsRelat_ToYear->currentText(),
                this->ui->box_StatsRelat_ToMonth->currentText() ) );
        this->ui->box_StatsRelat_ToDay->setCurrentIndex( 0 );
    }
    this->checkStatsRelatDrawable();
}

void MainWindow::on_box_StatsRelat_ToDay_currentIndexChanged(int index)
{
    this->checkStatsRelatDrawable();
}


void MainWindow::on_button_StatsRelat_Draw_clicked()
{
    if ( this->checkDataDB() == true ) {
        int aux;
        QString filter1, filter2;
        aux = this->ui->box_StatsRelat_LogsField_1->currentIndex();
        if ( aux == 0 ) {
            filter1 = this->crapview.parseBooleanFilter( this->ui->inLine_StatsRelat_Filter_1->text() );
        } else if ( aux >= 5 && aux <= 8 ) {
            filter1 = this->crapview.parseNumericFilter( this->ui->inLine_StatsRelat_Filter_1->text() );
        } else {
            filter1 = this->ui->inLine_StatsRelat_Filter_1->text();
        }
        aux = this->ui->box_StatsRelat_LogsField_2->currentIndex();
        if ( aux == 0 ) {
            filter2 = this->crapview.parseBooleanFilter( this->ui->inLine_StatsRelat_Filter_2->text() );
        } else if ( aux >= 5 && aux <= 8 ) {
            filter2 = this->crapview.parseNumericFilter( this->ui->inLine_StatsRelat_Filter_2->text() );
        } else {
            filter2 = this->crapview.parseTextualFilter( this->ui->inLine_StatsRelat_Filter_2->text() );
        }
        this->crapview.drawRelat(
            this->ui->chart_StatsRelat,
            this->CHARTS_THEMES.at( this->charts_theme_id ), this->FONTS,
            this->ui->box_StatsRelat_WebServer->currentText(),
            this->ui->box_StatsRelat_FromYear->currentText(),
            this->ui->box_StatsRelat_FromMonth->currentText(),
            this->ui->box_StatsRelat_FromDay->currentText(),
            this->ui->box_StatsRelat_ToYear->currentText(),
            this->ui->box_StatsRelat_ToMonth->currentText(),
            this->ui->box_StatsRelat_ToDay->currentText(),
            this->ui->box_StatsRelat_LogsField_1->currentText(), filter1,
            this->ui->box_StatsRelat_LogsField_2->currentText(), filter2 );
    }
}



////////////////
//// GLOBAL ////
//
void MainWindow::makeStatsGlobals( const QString& web_server )
{
    if ( this->checkDataDB() == true ) {
        std::vector<std::tuple<QString,QString>> recur_list;
        std::vector<std::tuple<QString,QString>> traffic_list;
        std::vector<std::tuple<QString,QString>> perf_list;
        std::vector<QString> work_list;

        const bool result = this->crapview.calcGlobals(
            recur_list, traffic_list, perf_list, work_list,
            web_server );

        if ( result == true ) {
            this->ui->label_StatsGlob_Recur_Protocol_String->setText( std::get<0>( recur_list.at(0) ) );
            this->ui->label_StatsGlob_Recur_Protocol_Count->setText( std::get<1>( recur_list.at(0) ) );
            this->ui->label_StatsGlob_Recur_Method_String->setText( std::get<0>( recur_list.at(1) ) );
            this->ui->label_StatsGlob_Recur_Method_Count->setText( std::get<1>( recur_list.at(1) ) );
            this->ui->label_StatsGlob_Recur_URI_String->setText( std::get<0>( recur_list.at(2) ) );
            this->ui->label_StatsGlob_Recur_URI_Count->setText( std::get<1>( recur_list.at(2) ) );
            this->ui->label_StatsGlob_Recur_UserAgent_String->setText( std::get<0>( recur_list.at(3) ) );
            this->ui->label_StatsGlob_Recur_UserAgent_Count->setText( std::get<1>( recur_list.at(3) ) );

            this->ui->label_StatsGlob_Traffic_Date_String->setText( std::get<0>( traffic_list.at(0) ) );
            this->ui->label_StatsGlob_Traffic_Date_Count->setText( std::get<1>( traffic_list.at(0) ) );
            this->ui->label_StatsGlob_Traffic_Day_String->setText( std::get<0>( traffic_list.at(1) ) );
            this->ui->label_StatsGlob_Traffic_Day_Count->setText( std::get<1>( traffic_list.at(1) ) );
            this->ui->label_StatsGlob_Traffic_Hour_String->setText( std::get<0>( traffic_list.at(2) ) );
            this->ui->label_StatsGlob_Traffic_Hour_Count->setText( std::get<1>( traffic_list.at(2) ) );

            this->ui->label_StatsGlob_Perf_Time_Mean->setText( std::get<0>( perf_list.at(0) ) );
            this->ui->label_StatsGlob_Perf_Time_Max->setText( std::get<1>( perf_list.at(0) ) );
            this->ui->label_StatsGlob_Perf_Sent_Mean->setText( std::get<0>( perf_list.at(1) ) );
            this->ui->label_StatsGlob_Perf_Sent_Max->setText( std::get<1>( perf_list.at(1) ) );
            this->ui->label_StatsGlob_Perf_Received_Mean->setText( std::get<0>( perf_list.at(2) ) );
            this->ui->label_StatsGlob_Perf_Received_Max->setText( std::get<1>( perf_list.at(2) ) );

            this->ui->label_StatsGlob_Work_Req_Count->setText( work_list.at(0) );
            this->ui->label_StatsGlob_Work_Time_Count->setText( work_list.at(1) );
            this->ui->label_StatsGlob_Work_Sent_Count->setText( work_list.at(2) );

        } else {
            this->resetStatsGlobals();
        }
        recur_list.clear(); traffic_list.clear();
        perf_list.clear();  work_list.clear();

    } else {
        this->resetStatsGlobals();
    }
}

void MainWindow::resetStatsGlobals()
{
    this->ui->label_StatsGlob_Recur_Protocol_String->setText( "-" );
    this->ui->label_StatsGlob_Recur_Protocol_Count->setText( "0" );
    this->ui->label_StatsGlob_Recur_Method_String->setText( "-" );
    this->ui->label_StatsGlob_Recur_Method_Count->setText( "0" );
    this->ui->label_StatsGlob_Recur_URI_String->setText( "-" );
    this->ui->label_StatsGlob_Recur_URI_Count->setText( "0" );
    this->ui->label_StatsGlob_Recur_UserAgent_String->setText( "-" );
    this->ui->label_StatsGlob_Recur_UserAgent_Count->setText( "0" );

    this->ui->label_StatsGlob_Traffic_Date_String->setText( "-" );
    this->ui->label_StatsGlob_Traffic_Date_Count->setText( "0" );
    this->ui->label_StatsGlob_Traffic_Day_String->setText( "-" );
    this->ui->label_StatsGlob_Traffic_Day_Count->setText( "0" );
    this->ui->label_StatsGlob_Traffic_Hour_String->setText( "-" );
    this->ui->label_StatsGlob_Traffic_Hour_Count->setText( "0" );

    this->ui->label_StatsGlob_Perf_Time_Mean->setText( "-" );
    this->ui->label_StatsGlob_Perf_Time_Max->setText( "-" );
    this->ui->label_StatsGlob_Perf_Sent_Mean->setText( "-" );
    this->ui->label_StatsGlob_Perf_Sent_Max->setText( "-" );
    this->ui->label_StatsGlob_Perf_Received_Mean->setText( "-" );
    this->ui->label_StatsGlob_Perf_Received_Max->setText( "-" );

    this->ui->label_StatsGlob_Work_Req_Count->setText( "-" );
    this->ui->label_StatsGlob_Work_Time_Count->setText( "-" );
    this->ui->label_StatsGlob_Work_Sent_Count->setText( "-" );

    if ( this->ui->button_StatsGlob_Apache->isChecked() == true ) {
        this->ui->button_StatsGlob_Apache->setChecked( false );
    } else if ( this->ui->button_StatsGlob_Nginx->isChecked() == true ) {
        this->ui->button_StatsGlob_Nginx->setChecked( false );
    } else if ( this->ui->button_StatsGlob_Iis->isChecked() == true ) {
        this->ui->button_StatsGlob_Iis->setChecked( false );
    }
}



void MainWindow::on_button_StatsGlob_Apache_clicked()
{
    this->makeStatsGlobals( "Apache2" );
    if ( this->ui->button_StatsGlob_Apache->isFlat() == true ) {
        // un-flat
        this->ui->button_StatsGlob_Apache->setFlat( false );
        this->ui->button_StatsGlob_Nginx->setFlat( true );
        this->ui->button_StatsGlob_Iis->setFlat( true );
    }
}


void MainWindow::on_button_StatsGlob_Nginx_clicked()
{
    this->makeStatsGlobals( "Nginx" );
    if ( this->ui->button_StatsGlob_Nginx->isFlat() == true ) {
        // un-flat
        this->ui->button_StatsGlob_Nginx->setFlat( false );
        this->ui->button_StatsGlob_Apache->setFlat( true );
        this->ui->button_StatsGlob_Iis->setFlat( true );
    }
}


void MainWindow::on_button_StatsGlob_Iis_clicked()
{
    this->makeStatsGlobals( "IIS" );
    if ( this->ui->button_StatsGlob_Iis->isFlat() == true ) {
        // un-flat
        this->ui->button_StatsGlob_Iis->setFlat( false );
        this->ui->button_StatsGlob_Apache->setFlat( true );
        this->ui->button_StatsGlob_Nginx->setFlat( true );
    }
}



/////////////////////////
//////// CONFIGS ////////
/////////////////////////

/////////////////
//// GENERAL ////
/////////////////

////////////////
//// WINDOW ////
void MainWindow::on_checkBox_ConfWindow_Geometry_clicked(bool checked)
{
    this->remember_window = checked;
}

void MainWindow::on_box_ConfWindow_Theme_currentIndexChanged(int index)
{
    this->window_theme_id = index;
    this->updateUiTheme();
}


/////////////////
//// DIALOGS ////
void MainWindow::on_slider_ConfDialogs_General_sliderReleased()
{
    this->dialogs_level = this->ui->slider_ConfDialogs_General->value();
}
void MainWindow::on_slider_ConfDialogs_Logs_sliderReleased()
{
    this->craplog.setDialogsLevel( this->ui->slider_ConfDialogs_Logs->value() );
}
void MainWindow::on_slider_ConfDialogs_Stats_sliderReleased()
{
    this->crapview.setDialogsLevel( this->ui->slider_ConfDialogs_Stats->value() );
}


//////////////////////
//// TEXT BROWSER ////
void MainWindow::on_box_ConfTextBrowser_Font_currentIndexChanged(int index)
{
    QFont font;
    switch ( index ) {
        case 0:
            font = this->FONTS.at( "main" );
            break;
        case 1:
            font = this->FONTS.at( "alternative" );
            break;
        case 2:
            font = this->FONTS.at( "script" );
            break;
        default:
            throw GenericException( "Unexpected Font index: "+std::to_string(index) );
    }
    this->TB.setFont( font );
    this->TB.setFontFamily( this->ui->box_ConfTextBrowser_Font->currentText() );
    this->crapnote->setTextFont( font );
    this->ui->textBrowser_ConfTextBrowser_Preview->setFont( font );
}
void MainWindow::on_checkBox_ConfTextBrowser_WideLines_clicked(bool checked)
{
    this->TB.setWideLinesUsage( checked );
    this->refreshTextBrowserPreview();
}
void MainWindow::on_box_ConfTextBrowser_ColorScheme_currentIndexChanged(int index)
{
    this->TB.setColorScheme( index, this->TB_COLOR_SCHEMES.at( index ) );
    this->crapnote->setColorScheme( index );
    this->refreshTextBrowserPreview();
}
void MainWindow::refreshTextBrowserPreview()
{
    QString content = "";
    this->TB.makePreview( content );
    this->ui->textBrowser_ConfTextBrowser_Preview->setText( content );
    this->ui->textBrowser_ConfTextBrowser_Preview->setFont( this->TB.getFont() );
}


////////////////
//// CHARTS ////
void MainWindow::on_box_ConfCharts_Theme_currentIndexChanged(int index)
{
    this->charts_theme_id = index;
    this->refreshChartsPreview();
}
void MainWindow::refreshChartsPreview()
{
    QColor col = Qt::GlobalColor::darkGreen;
    QBarSet *bars_1 = new QBarSet( "" );
    bars_1->setColor( col );
    QBarSet *bars_2 = new QBarSet( "" );
    bars_2->setColor( col );
    QBarSet *bars_3 = new QBarSet( "Infoes" );
    bars_3->setColor( col );
    QBarSet *bars_4 = new QBarSet( "" );
    bars_4->setColor( col );
    QBarSet *bars_5 = new QBarSet( "" );
    bars_5->setColor( col );
    QBarSet *bars_6 = new QBarSet( "" );
    bars_6->setColor( col );

    int aux, max=0;
    for ( int i=0; i<24; i++ ) {
        aux = rand() %100; *bars_1 << aux;
        if ( aux > max ) { max = aux; }
        aux = rand() %100; *bars_2 << aux;
        if ( aux > max ) { max = aux; }
        aux = rand() %100; *bars_3 << aux;
        if ( aux > max ) { max = aux; }
        aux = rand() %100; *bars_4 << aux;
        if ( aux > max ) { max = aux; }
        aux = rand() %100; *bars_5 << aux;
        if ( aux > max ) { max = aux; }
        aux = rand() %100; *bars_6 << aux;
        if ( aux > max ) { max = aux; }
    }

    QBarSeries *bars = new QBarSeries();
    bars->append( bars_1 ); bars->append( bars_2 ); bars->append( bars_3 );
    bars->append( bars_4 ); bars->append( bars_5 ); bars->append( bars_6 );
    bars->setBarWidth( 1 );

    QChart *t_chart = new QChart();
    // apply the theme
    t_chart->setTheme( this->CHARTS_THEMES.at( this->charts_theme_id ) );
    // add the bars
    t_chart->addSeries( bars );
    t_chart->setTitle( "Sample preview" );
    t_chart->setTitleFont( this->FONTS.at("main") );
    t_chart->legend()->setFont( this->FONTS.at("main_small") );
    t_chart->setAnimationOptions( QChart::SeriesAnimations );

    QStringList categories;
    categories << "00" << "01" << "02" << "03" << "04" << "05" << "06" << "07" << "08" << "09" << "10" << "11"
               << "12" << "13" << "14" << "15" << "16" << "17" << "18" << "19" << "20" << "21" << "22" << "23";

    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append( categories );
    axisX->setLabelsFont( this->FONTS.at( "main_small" ) );
    t_chart->addAxis( axisX, Qt::AlignBottom );
    bars->attachAxis( axisX );

    QValueAxis *axisY = new QValueAxis();
    axisY->setLabelFormat( "%d" );
    axisY->setRange( 0, max );
    axisY->setLabelsFont( this->FONTS.at( "main_small" ) );
    t_chart->addAxis( axisY, Qt::AlignLeft );
    bars->attachAxis( axisY) ;

    t_chart->legend()->setVisible( true );
    t_chart->legend()->setAlignment( Qt::AlignBottom );

    this->ui->chart_ConfCharts_Preview->setChart( t_chart );
    this->ui->chart_ConfCharts_Preview->setRenderHint( QPainter::Antialiasing );
}


///////////////////
//// DATABASES ////
// data collection
void MainWindow::on_inLine_ConfDatabases_Data_Path_textChanged(const QString &arg1)
{
    if ( arg1.size() > 0 ) {
        std::string path = this->resolvePath( arg1.toStdString() );
        if ( IOutils::checkDir( path ) == true ) {
            this->ui->icon_ConfDatabases_Data_Wrong->setVisible( false );
            this->ui->button_ConfDatabases_Data_Save->setEnabled( true );
        } else {
            this->ui->icon_ConfDatabases_Data_Wrong->setVisible( true );
            this->ui->button_ConfDatabases_Data_Save->setEnabled( false );
        }
    } else {
        this->ui->icon_ConfDatabases_Data_Wrong->setVisible( true );
        this->ui->button_ConfDatabases_Data_Save->setEnabled( false );
    }
}
void MainWindow::on_inLine_ConfDatabases_Data_Path_returnPressed()
{
    this->on_button_ConfDatabases_Data_Save_clicked();
}
void MainWindow::on_button_ConfDatabases_Data_Save_clicked()
{
    if ( this->ui->icon_ConfDatabases_Data_Wrong->isVisible() == false ) {
        // set the paths
        std::string path = this->resolvePath( this->ui->inLine_ConfDatabases_Data_Path->text().toStdString() );
        if ( StringOps::endsWith( path, "/" ) ) {
            path = StringOps::rstrip( path, "/" );
        }
        if ( IOutils::checkDir( path, true ) == false ) {
            DialogSec::warnDirNotReadable( nullptr );
        }
        if ( IOutils::checkDir( path, false, true ) == false ) {
            DialogSec::warnDirNotWritable( nullptr );
        }
        this->db_data_path = path;
        this->craplog.setStatsDatabasePath( path );
        this->crapview.setDbPath( path );
        this->ui->inLine_ConfDatabases_Data_Path->setText( QString::fromStdString( path ) );
    }
    this->ui->button_ConfDatabases_Data_Save->setEnabled( false );
}

// usef files hashes
void MainWindow::on_inLine_ConfDatabases_Hashes_Path_textChanged(const QString &arg1)
{
    if ( arg1.size() > 0 ) {
        std::string path = this->resolvePath( arg1.toStdString() );
        if ( IOutils::checkDir( path ) == true ) {
            this->ui->icon_ConfDatabases_Hashes_Wrong->setVisible( false );
            this->ui->button_ConfDatabases_Hashes_Save->setEnabled( true );
        } else {
            this->ui->icon_ConfDatabases_Hashes_Wrong->setVisible( true );
            this->ui->button_ConfDatabases_Hashes_Save->setEnabled( false );
        }
    } else {
        this->ui->icon_ConfDatabases_Hashes_Wrong->setVisible( true );
        this->ui->button_ConfDatabases_Hashes_Save->setEnabled( false );
    }
}
void MainWindow::on_inLine_ConfDatabases_Hashes_Path_returnPressed()
{
    this->on_button_ConfDatabases_Hashes_Save_clicked();
}
void MainWindow::on_button_ConfDatabases_Hashes_Save_clicked()
{
    if ( this->ui->icon_ConfDatabases_Hashes_Wrong->isVisible() == false ) {
        // set the paths
        std::string path = this->resolvePath( this->ui->inLine_ConfDatabases_Hashes_Path->text().toStdString() );
        if ( StringOps::endsWith( path, "/" ) ) {
            path = StringOps::rstrip( path, "/" );
        }
        if ( IOutils::checkDir( path, true ) == false ) {
            DialogSec::warnDirNotReadable( nullptr );
        }
        if ( IOutils::checkDir( path, false, true ) == false ) {
            DialogSec::warnDirNotWritable( nullptr );
        }
        this->db_hashes_path = path;
        this->craplog.setHashesDatabasePath( path );
        this->ui->inLine_ConfDatabases_Hashes_Path->setText( QString::fromStdString( path ) );
    }
    this->ui->button_ConfDatabases_Hashes_Save->setEnabled( false );
}


//////////////
//// LOGS ////
//////////////

//////////////////
//// DEFAULTS ////
void MainWindow::on_radio_ConfDefaults_Apache_toggled(bool checked)
{
    this->default_ws = this->APACHE_ID;
}
void MainWindow::on_radio_ConfDefaults_Nginx_toggled(bool checked)
{
    this->default_ws = this->NGINX_ID;
}
void MainWindow::on_radio_ConfDefaults_Iis_toggled(bool checked)
{
    this->default_ws = this->IIS_ID;
}

/////////////////
//// CONTROL ////
void MainWindow::on_checkBox_ConfControl_Usage_clicked(bool checked)
{
    this->hide_used_files = checked;
}
void MainWindow::on_checkBox_ConfControl_Size_clicked(bool checked)
{
    if ( checked == false ) {
        // disable size warning
        this->ui->spinBox_ConfControl_Size->setEnabled( false );
        this->craplog.setWarningSize( 0 );
    } else {
        // enable warning
        this->ui->spinBox_ConfControl_Size->setEnabled( true );
        this->craplog.setWarningSize( (this->ui->spinBox_ConfControl_Size->value() * 1'048'576) +1 );
    }
}
void MainWindow::on_spinBox_ConfControl_Size_editingFinished()
{
    this->craplog.setWarningSize( (this->ui->spinBox_ConfControl_Size->value() * 1'048'576) +1 );
}


////////////////
//// APACHE ////
// paths
void MainWindow::on_inLine_ConfApache_Path_String_textChanged(const QString &arg1)
{
    if ( arg1.size() > 0 ) {
        std::string path = this->resolvePath( arg1.toStdString() );
        if ( IOutils::checkDir( path ) == true ) {
            this->ui->icon_ConfApache_Path_Wrong->setVisible( false );
            this->ui->button_ConfApache_Path_Save->setEnabled( true );
        } else {
            this->ui->icon_ConfApache_Path_Wrong->setVisible( true );
            this->ui->button_ConfApache_Path_Save->setEnabled( false );
        }
    } else {
        this->ui->icon_ConfApache_Path_Wrong->setVisible( true );
        this->ui->button_ConfApache_Path_Save->setEnabled( false );
    }
}
void MainWindow::on_inLine_ConfApache_Path_String_returnPressed()
{
    this->on_button_ConfApache_Path_Save_clicked();
}
void MainWindow::on_button_ConfApache_Path_Save_clicked()
{
    if ( this->ui->icon_ConfApache_Path_Wrong->isVisible() == false ) {
        // set the paths
        std::string path = this->resolvePath( this->ui->inLine_ConfApache_Path_String->text().toStdString() );
        if ( StringOps::endsWith( path, "/" ) ) {
            path = StringOps::rstrip( path, "/" );
        }
        if ( IOutils::checkDir( path, true ) == false ) {
            DialogSec::warnDirNotReadable( nullptr );
        }
        this->craplog.setLogsPath( this->APACHE_ID, path );
        this->ui->inLine_ConfApache_Path_String->setText( QString::fromStdString( path ) );
    }
    this->ui->button_ConfApache_Path_Save->setEnabled( false );
}

// formats
void MainWindow::on_inLine_ConfApache_Format_String_cursorPositionChanged(int arg1, int arg2)
{
    if ( arg2 > 0 ) {
        this->ui->button_ConfApache_Format_Save->setEnabled( true );
    } else {
        this->ui->button_ConfApache_Format_Save->setEnabled( false );
    }
}
void MainWindow::on_inLine_ConfApache_Format_String_returnPressed()
{
    if ( this->ui->button_ConfApache_Format_Save->isEnabled() == true ) {
        this->on_button_ConfApache_Format_Save_clicked();
    }
}
void MainWindow::on_button_ConfApache_Format_Save_clicked()
{
    this->craplog.setApacheLogFormat( this->ui->inLine_ConfApache_Format_String->text().toStdString() );
    this->ui->button_ConfApache_Format_Save->setEnabled( false );
}
void MainWindow::on_button_ConfApache_Format_Sample_clicked()
{
    this->ui->preview_ConfApache_Format_Sample->setText(
        this->craplog.getLogsFormatSample( this->APACHE_ID ) );
}
void MainWindow::on_button_ConfApache_Format_Help_clicked()
{
    this->showHelp( "apache_format" );
}

// warnlists
void MainWindow::on_box_ConfApache_Warnlist_Field_currentTextChanged(const QString &arg1)
{
    this->ui->inLine_ConfApache_Warnlist_String->clear();
    this->ui->list_ConfApache_Warnlist_List->clear();
    // update the list
    const std::vector<std::string>& list = this->craplog.getWarnlist(
        this->APACHE_ID, this->crapview.getLogFieldID( arg1 ) );
    for ( const std::string& item : list ) {
        this->ui->list_ConfApache_Warnlist_List->addItem( QString::fromStdString( item ) );
    }
    // check/uncheck the usage option
    bool used = this->craplog.isWarnlistUsed(
        this->APACHE_ID,
        this->crapview.getLogFieldID( this->ui->box_ConfApache_Warnlist_Field->currentText() ) );
    this->ui->checkBox_ConfApache_Warnlist_Used->setChecked( used );
    this->on_checkBox_ConfApache_Warnlist_Used_clicked( used );
}
void MainWindow::on_checkBox_ConfApache_Warnlist_Used_clicked(bool checked)
{
    this->craplog.setWarnlistUsed(
        this->APACHE_ID,
        this->crapview.getLogFieldID( this->ui->box_ConfApache_Warnlist_Field->currentText() ),
        checked );
    if ( checked == true ) {
        this->ui->inLine_ConfApache_Warnlist_String->setEnabled( true );
        this->ui->list_ConfApache_Warnlist_List->setEnabled( true );
    } else {
        this->ui->inLine_ConfApache_Warnlist_String->clear();
        this->ui->inLine_ConfApache_Warnlist_String->setEnabled( false );
        this->ui->list_ConfApache_Warnlist_List->clearSelection();
        this->ui->list_ConfApache_Warnlist_List->setEnabled( false );
    }
}

void MainWindow::on_inLine_ConfApache_Warnlist_String_cursorPositionChanged(int arg1, int arg2)
{
    if ( arg2 > 0 ) {
        this->ui->button_ConfApache_Warnlist_Add->setEnabled( true );
    } else {
        this->ui->button_ConfApache_Warnlist_Add->setEnabled( false );
    }
}
void MainWindow::on_inLine_ConfApache_Warnlist_String_returnPressed()
{
    this->on_button_ConfApache_Warnlist_Add_clicked();
}
void MainWindow::on_button_ConfApache_Warnlist_Add_clicked()
{
    const QString& item = this->ui->inLine_ConfApache_Warnlist_String->text();
    if ( this->ui->list_ConfApache_Warnlist_List->findItems( item, Qt::MatchFlag::MatchCaseSensitive ).size() == 0 ) {
        // not in the list yet, append
        this->ui->list_ConfApache_Warnlist_List->addItem( item );
        this->craplog.warnlistAdd(
            this->APACHE_ID,
            this->crapview.getLogFieldID( this->ui->box_ConfApache_Warnlist_Field->currentText() ),
            item.toStdString() );
    }
    // select the item in the list, in both cases it was already in or it has been just inserted
    this->ui->list_ConfApache_Warnlist_List->clearSelection();
    this->ui->list_ConfApache_Warnlist_List->findItems( item, Qt::MatchFlag::MatchCaseSensitive ).at(0)->setSelected( true );
    this->ui->inLine_ConfApache_Warnlist_String->clear();
}

void MainWindow::on_list_ConfApache_Warnlist_List_itemSelectionChanged()
{
    if ( this->ui->list_ConfApache_Warnlist_List->selectedItems().size() == 1 ) {
        this->ui->button_ConfApache_Warnlist_Remove->setEnabled( true );
        this->ui->button_ConfApache_Warnlist_Up->setEnabled( true );
        this->ui->button_ConfApache_Warnlist_Down->setEnabled( true );
        // polishing
        const auto& item = this->ui->list_ConfApache_Warnlist_List->selectedItems().at(0);
        const int max = this->ui->list_ConfApache_Warnlist_List->count() -1;
        if ( max == 0 ) {
            this->ui->button_ConfApache_Warnlist_Up->setEnabled( false );
            this->ui->button_ConfApache_Warnlist_Down->setEnabled( false );
        } else {
            for ( int i=0; i<=max; i++ ) {
                if ( this->ui->list_ConfApache_Warnlist_List->item(i) == item ) {
                    if ( i == 0 ) {
                        this->ui->button_ConfApache_Warnlist_Up->setEnabled( false );
                    } else if ( i == max ) {
                        this->ui->button_ConfApache_Warnlist_Down->setEnabled( false );
                    }
                }
            }
        }
    } else {
        this->ui->button_ConfApache_Warnlist_Remove->setEnabled( false );
        this->ui->button_ConfApache_Warnlist_Up->setEnabled( false );
        this->ui->button_ConfApache_Warnlist_Down->setEnabled( false );
    }
}
void MainWindow::on_button_ConfApache_Warnlist_Remove_clicked()
{
    const auto& item = this->ui->list_ConfApache_Warnlist_List->selectedItems().at(0);
    this->craplog.warnlistRemove(
        this->APACHE_ID,
        this->crapview.getLogFieldID( this->ui->box_ConfApache_Warnlist_Field->currentText() ),
        item->text().toStdString() );
    // refresh the list
    this->on_box_ConfApache_Warnlist_Field_currentTextChanged( this->ui->box_ConfApache_Warnlist_Field->currentText() );
}
void MainWindow::on_button_ConfApache_Warnlist_Up_clicked()
{
    const auto& item = this->ui->list_ConfApache_Warnlist_List->selectedItems().at(0);
    const int i = this->craplog.warnlistMoveUp(
        this->APACHE_ID,
        this->crapview.getLogFieldID( this->ui->box_ConfApache_Warnlist_Field->currentText() ),
        item->text().toStdString() );
    // refresh the list
    this->on_box_ConfApache_Warnlist_Field_currentTextChanged( this->ui->box_ConfApache_Warnlist_Field->currentText() );
    // re-select the item
    this->ui->list_ConfApache_Warnlist_List->item( i )->setSelected( true );
    this->ui->list_ConfApache_Warnlist_List->setFocus();
}
void MainWindow::on_button_ConfApache_Warnlist_Down_clicked()
{
    const auto& item = this->ui->list_ConfApache_Warnlist_List->selectedItems().at(0);
    const int i = this->craplog.warnlistMoveDown(
        this->APACHE_ID,
        this->crapview.getLogFieldID( this->ui->box_ConfApache_Warnlist_Field->currentText() ),
        item->text().toStdString() );
    // refresh the list
    this->on_box_ConfApache_Warnlist_Field_currentTextChanged( this->ui->box_ConfApache_Warnlist_Field->currentText() );
    // re-select the item
    this->ui->list_ConfApache_Warnlist_List->item( i )->setSelected( true );
    this->ui->list_ConfApache_Warnlist_List->setFocus();
}


// blacklist
void MainWindow::on_box_ConfApache_Blacklist_Field_currentTextChanged(const QString &arg1)
{
    this->ui->inLine_ConfApache_Blacklist_String->clear();
    this->ui->list_ConfApache_Blacklist_List->clear();
    // update the list
    const std::vector<std::string>& list = this->craplog.getBlacklist(
        this->APACHE_ID, this->crapview.getLogFieldID( arg1 ) );
    for ( const std::string& item : list ) {
        this->ui->list_ConfApache_Blacklist_List->addItem( QString::fromStdString( item ) );
    }
    // check/uncheck the usage option
    bool used = this->craplog.isBlacklistUsed(
        this->APACHE_ID,
        this->crapview.getLogFieldID( this->ui->box_ConfApache_Blacklist_Field->currentText() ) );
    this->ui->checkBox_ConfApache_Blacklist_Used->setChecked( used );
    this->on_checkBox_ConfApache_Blacklist_Used_clicked( used );
}
void MainWindow::on_checkBox_ConfApache_Blacklist_Used_clicked(bool checked)
{
    this->craplog.setBlacklistUsed(
        this->APACHE_ID,
        this->crapview.getLogFieldID( this->ui->box_ConfApache_Blacklist_Field->currentText() ),
        checked );
    if ( checked == true ) {
        this->ui->inLine_ConfApache_Blacklist_String->setEnabled( true );
        this->ui->list_ConfApache_Blacklist_List->setEnabled( true );
    } else {
        this->ui->inLine_ConfApache_Blacklist_String->clear();
        this->ui->inLine_ConfApache_Blacklist_String->setEnabled( false );
        this->ui->list_ConfApache_Blacklist_List->clearSelection();
        this->ui->list_ConfApache_Blacklist_List->setEnabled( false );
    }
}

void MainWindow::on_inLine_ConfApache_Blacklist_String_cursorPositionChanged(int arg1, int arg2)
{
    if ( arg2 > 0 ) {
        this->ui->button_ConfApache_Blacklist_Add->setEnabled( true );
    } else {
        this->ui->button_ConfApache_Blacklist_Add->setEnabled( false );
    }
}
void MainWindow::on_inLine_ConfApache_Blacklist_String_returnPressed()
{
    this->on_button_ConfApache_Blacklist_Add_clicked();
}
void MainWindow::on_button_ConfApache_Blacklist_Add_clicked()
{
    const QString& item = this->ui->inLine_ConfApache_Blacklist_String->text();
    if ( this->ui->list_ConfApache_Blacklist_List->findItems( item, Qt::MatchFlag::MatchCaseSensitive ).size() == 0 ) {
        // not in the list yet, append
        this->ui->list_ConfApache_Blacklist_List->addItem( item );
        this->craplog.blacklistAdd(
            this->APACHE_ID,
            this->crapview.getLogFieldID( this->ui->box_ConfApache_Blacklist_Field->currentText() ),
            item.toStdString() );
    }
    // select the item in the list, in both cases it was already in or it has been just inserted
    this->ui->list_ConfApache_Blacklist_List->clearSelection();
    this->ui->list_ConfApache_Blacklist_List->findItems( item, Qt::MatchFlag::MatchCaseSensitive ).at(0)->setSelected( true );
    this->ui->inLine_ConfApache_Blacklist_String->clear();
}

void MainWindow::on_list_ConfApache_Blacklist_List_itemSelectionChanged()
{
    if ( this->ui->list_ConfApache_Blacklist_List->selectedItems().size() == 1 ) {
        this->ui->button_ConfApache_Blacklist_Remove->setEnabled( true );
        this->ui->button_ConfApache_Blacklist_Up->setEnabled( true );
        this->ui->button_ConfApache_Blacklist_Down->setEnabled( true );
        // polishing
        const auto& item = this->ui->list_ConfApache_Blacklist_List->selectedItems().at(0);
        const int max = this->ui->list_ConfApache_Blacklist_List->count() -1;
        if ( max == 0 ) {
            this->ui->button_ConfApache_Blacklist_Up->setEnabled( false );
            this->ui->button_ConfApache_Blacklist_Down->setEnabled( false );
        } else {
            for ( int i=0; i<=max; i++ ) {
                if ( this->ui->list_ConfApache_Blacklist_List->item(i) == item ) {
                    if ( i == 0 ) {
                        this->ui->button_ConfApache_Blacklist_Up->setEnabled( false );
                    } else if ( i == max ) {
                        this->ui->button_ConfApache_Blacklist_Down->setEnabled( false );
                    }
                }
            }
        }
    } else {
        this->ui->button_ConfApache_Blacklist_Remove->setEnabled( false );
        this->ui->button_ConfApache_Blacklist_Up->setEnabled( false );
        this->ui->button_ConfApache_Blacklist_Down->setEnabled( false );
    }
}
void MainWindow::on_button_ConfApache_Blacklist_Remove_clicked()
{
    const auto& item = this->ui->list_ConfApache_Blacklist_List->selectedItems().at(0);
    this->craplog.blacklistRemove(
        this->APACHE_ID,
        this->crapview.getLogFieldID( this->ui->box_ConfApache_Blacklist_Field->currentText() ),
        item->text().toStdString() );
    // refresh the list
    this->on_box_ConfApache_Blacklist_Field_currentTextChanged( this->ui->box_ConfApache_Blacklist_Field->currentText() );
}
void MainWindow::on_button_ConfApache_Blacklist_Up_clicked()
{
    const auto& item = this->ui->list_ConfApache_Blacklist_List->selectedItems().at(0);
    const int i = this->craplog.blacklistMoveUp(
        this->APACHE_ID,
        this->crapview.getLogFieldID( this->ui->box_ConfApache_Blacklist_Field->currentText() ),
        item->text().toStdString() );
    // refresh the list
    this->on_box_ConfApache_Blacklist_Field_currentTextChanged( this->ui->box_ConfApache_Blacklist_Field->currentText() );
    // re-select the item
    this->ui->list_ConfApache_Blacklist_List->item( i )->setSelected( true );
    this->ui->list_ConfApache_Blacklist_List->setFocus();
}
void MainWindow::on_button_ConfApache_Blacklist_Down_clicked()
{
    const auto& item = this->ui->list_ConfApache_Blacklist_List->selectedItems().at(0);
    const int i = this->craplog.blacklistMoveDown(
        this->APACHE_ID,
        this->crapview.getLogFieldID( this->ui->box_ConfApache_Blacklist_Field->currentText() ),
        item->text().toStdString() );
    // refresh the list
    this->on_box_ConfApache_Blacklist_Field_currentTextChanged( this->ui->box_ConfApache_Blacklist_Field->currentText() );
    // re-select the item
    this->ui->list_ConfApache_Blacklist_List->item( i )->setSelected( true );
    this->ui->list_ConfApache_Blacklist_List->setFocus();
}


////////////////
//// NGINX ////
// paths
void MainWindow::on_inLine_ConfNginx_Path_String_textChanged(const QString &arg1)
{
    if ( arg1.size() > 0 ) {
        std::string path = this->resolvePath( arg1.toStdString() );
        if ( IOutils::checkDir( path ) == true ) {
            this->ui->icon_ConfNginx_Path_Wrong->setVisible( false );
            this->ui->button_ConfNginx_Path_Save->setEnabled( true );
        } else {
            this->ui->icon_ConfNginx_Path_Wrong->setVisible( true );
            this->ui->button_ConfNginx_Path_Save->setEnabled( false );
        }
    } else {
        this->ui->icon_ConfNginx_Path_Wrong->setVisible( true );
        this->ui->button_ConfNginx_Path_Save->setEnabled( false );
    }
}
void MainWindow::on_inLine_ConfNginx_Path_String_returnPressed()
{
    this->on_button_ConfNginx_Path_Save_clicked();
}
void MainWindow::on_button_ConfNginx_Path_Save_clicked()
{
    if ( this->ui->icon_ConfNginx_Path_Wrong->isVisible() == false ) {
        // set the paths
        std::string path = this->resolvePath( this->ui->inLine_ConfNginx_Path_String->text().toStdString() );
        if ( StringOps::endsWith( path, "/" ) ) {
            path = StringOps::rstrip( path, "/" );
        }
        if ( IOutils::checkDir( path, true ) == false ) {
            DialogSec::warnDirNotReadable( nullptr );
        }
        this->craplog.setLogsPath( this->NGINX_ID, path );
        this->ui->inLine_ConfNginx_Path_String->setText( QString::fromStdString( path ) );
    }
    this->ui->button_ConfNginx_Path_Save->setEnabled( false );
}

// formats
void MainWindow::on_inLine_ConfNginx_Format_String_cursorPositionChanged(int arg1, int arg2)
{
    if ( arg2 > 0 ) {
        this->ui->button_ConfNginx_Format_Save->setEnabled( true );
    } else {
        this->ui->button_ConfNginx_Format_Save->setEnabled( false );
    }
}
void MainWindow::on_inLine_ConfNginx_Format_String_returnPressed()
{
    if ( this->ui->button_ConfNginx_Format_Save->isEnabled() == true ) {
        this->on_button_ConfNginx_Format_Save_clicked();
    }
}
void MainWindow::on_button_ConfNginx_Format_Save_clicked()
{
    this->craplog.setNginxLogFormat( this->ui->inLine_ConfNginx_Format_String->text().toStdString() );
    this->ui->button_ConfNginx_Format_Save->setEnabled( false );
}
void MainWindow::on_button_ConfNginx_Format_Sample_clicked()
{
    this->ui->preview_ConfNginx_Format_Sample->setText(
        this->craplog.getLogsFormatSample( this->NGINX_ID ) );
}
void MainWindow::on_button_ConfNginx_Format_Help_clicked()
{
    this->showHelp( "nginx_format" );
}

// warnlists
void MainWindow::on_box_ConfNginx_Warnlist_Field_currentTextChanged(const QString &arg1)
{
    this->ui->inLine_ConfNginx_Warnlist_String->clear();
    this->ui->list_ConfNginx_Warnlist_List->clear();
    // update the list
    const std::vector<std::string>& list = this->craplog.getWarnlist(
        this->NGINX_ID, this->crapview.getLogFieldID( arg1 ) );
    for ( const std::string& item : list ) {
        this->ui->list_ConfNginx_Warnlist_List->addItem( QString::fromStdString( item ) );
    }
    // check/uncheck the usage option
    bool used = this->craplog.isWarnlistUsed(
        this->NGINX_ID,
        this->crapview.getLogFieldID( this->ui->box_ConfNginx_Warnlist_Field->currentText() ) );
    this->ui->checkBox_ConfNginx_Warnlist_Used->setChecked( used );
    this->on_checkBox_ConfNginx_Warnlist_Used_clicked( used );
}
void MainWindow::on_checkBox_ConfNginx_Warnlist_Used_clicked(bool checked)
{
    this->craplog.setWarnlistUsed(
        this->NGINX_ID,
        this->crapview.getLogFieldID( this->ui->box_ConfNginx_Warnlist_Field->currentText() ),
        checked );
    if ( checked == true ) {
        this->ui->inLine_ConfNginx_Warnlist_String->setEnabled( true );
        this->ui->list_ConfNginx_Warnlist_List->setEnabled( true );
    } else {
        this->ui->inLine_ConfNginx_Warnlist_String->clear();
        this->ui->inLine_ConfNginx_Warnlist_String->setEnabled( false );
        this->ui->list_ConfNginx_Warnlist_List->clearSelection();
        this->ui->list_ConfNginx_Warnlist_List->setEnabled( false );
    }
}

void MainWindow::on_inLine_ConfNginx_Warnlist_String_cursorPositionChanged(int arg1, int arg2)
{
    if ( arg2 > 0 ) {
        this->ui->button_ConfNginx_Warnlist_Add->setEnabled( true );
    } else {
        this->ui->button_ConfNginx_Warnlist_Add->setEnabled( false );
    }
}
void MainWindow::on_inLine_ConfNginx_Warnlist_String_returnPressed()
{
    this->on_button_ConfNginx_Warnlist_Add_clicked();
}
void MainWindow::on_button_ConfNginx_Warnlist_Add_clicked()
{
    const QString& item = this->ui->inLine_ConfNginx_Warnlist_String->text();
    if ( this->ui->list_ConfNginx_Warnlist_List->findItems( item, Qt::MatchFlag::MatchCaseSensitive ).size() == 0 ) {
        // not in the list yet, append
        this->ui->list_ConfNginx_Warnlist_List->addItem( item );
        this->craplog.warnlistAdd(
            this->NGINX_ID,
            this->crapview.getLogFieldID( this->ui->box_ConfNginx_Warnlist_Field->currentText() ),
            item.toStdString() );
    }
    // select the item in the list, in both cases it was already in or it has been just inserted
    this->ui->list_ConfNginx_Warnlist_List->clearSelection();
    this->ui->list_ConfNginx_Warnlist_List->findItems( item, Qt::MatchFlag::MatchCaseSensitive ).at(0)->setSelected( true );
    this->ui->inLine_ConfNginx_Warnlist_String->clear();
}

void MainWindow::on_list_ConfNginx_Warnlist_List_itemSelectionChanged()
{
    if ( this->ui->list_ConfNginx_Warnlist_List->selectedItems().size() == 1 ) {
        this->ui->button_ConfNginx_Warnlist_Remove->setEnabled( true );
        this->ui->button_ConfNginx_Warnlist_Up->setEnabled( true );
        this->ui->button_ConfNginx_Warnlist_Down->setEnabled( true );
        // polishing
        const auto& item = this->ui->list_ConfNginx_Warnlist_List->selectedItems().at(0);
        const int max = this->ui->list_ConfNginx_Warnlist_List->count() -1;
        if ( max == 0 ) {
            this->ui->button_ConfNginx_Warnlist_Up->setEnabled( false );
            this->ui->button_ConfNginx_Warnlist_Down->setEnabled( false );
        } else {
            for ( int i=0; i<=max; i++ ) {
                if ( this->ui->list_ConfNginx_Warnlist_List->item(i) == item ) {
                    if ( i == 0 ) {
                        this->ui->button_ConfNginx_Warnlist_Up->setEnabled( false );
                    } else if ( i == max ) {
                        this->ui->button_ConfNginx_Warnlist_Down->setEnabled( false );
                    }
                }
            }
        }
    } else {
        this->ui->button_ConfNginx_Warnlist_Remove->setEnabled( false );
        this->ui->button_ConfNginx_Warnlist_Up->setEnabled( false );
        this->ui->button_ConfNginx_Warnlist_Down->setEnabled( false );
    }
}
void MainWindow::on_button_ConfNginx_Warnlist_Remove_clicked()
{
    const auto& item = this->ui->list_ConfNginx_Warnlist_List->selectedItems().at(0);
    this->craplog.warnlistRemove(
        this->NGINX_ID,
        this->crapview.getLogFieldID( this->ui->box_ConfNginx_Warnlist_Field->currentText() ),
        item->text().toStdString() );
    // refresh the list
    this->on_box_ConfNginx_Warnlist_Field_currentTextChanged( this->ui->box_ConfNginx_Warnlist_Field->currentText() );
}
void MainWindow::on_button_ConfNginx_Warnlist_Up_clicked()
{
    const auto& item = this->ui->list_ConfNginx_Warnlist_List->selectedItems().at(0);
    const int i = this->craplog.warnlistMoveUp(
        this->NGINX_ID,
        this->crapview.getLogFieldID( this->ui->box_ConfNginx_Warnlist_Field->currentText() ),
        item->text().toStdString() );
    // refresh the list
    this->on_box_ConfNginx_Warnlist_Field_currentTextChanged( this->ui->box_ConfNginx_Warnlist_Field->currentText() );
    // re-select the item
    this->ui->list_ConfNginx_Warnlist_List->item( i )->setSelected( true );
    this->ui->list_ConfNginx_Warnlist_List->setFocus();
}
void MainWindow::on_button_ConfNginx_Warnlist_Down_clicked()
{
    const auto& item = this->ui->list_ConfNginx_Warnlist_List->selectedItems().at(0);
    const int i = this->craplog.warnlistMoveDown(
        this->NGINX_ID,
        this->crapview.getLogFieldID( this->ui->box_ConfNginx_Warnlist_Field->currentText() ),
        item->text().toStdString() );
    // refresh the list
    this->on_box_ConfNginx_Warnlist_Field_currentTextChanged( this->ui->box_ConfNginx_Warnlist_Field->currentText() );
    // re-select the item
    this->ui->list_ConfNginx_Warnlist_List->item( i )->setSelected( true );
    this->ui->list_ConfNginx_Warnlist_List->setFocus();
}


// blacklist
void MainWindow::on_box_ConfNginx_Blacklist_Field_currentTextChanged(const QString &arg1)
{
    this->ui->inLine_ConfNginx_Blacklist_String->clear();
    this->ui->list_ConfNginx_Blacklist_List->clear();
    // update the list
    const std::vector<std::string>& list = this->craplog.getBlacklist(
        this->NGINX_ID, this->crapview.getLogFieldID( arg1 ) );
    for ( const std::string& item : list ) {
        this->ui->list_ConfNginx_Blacklist_List->addItem( QString::fromStdString( item ) );
    }
    // check/uncheck the usage option
    bool used = this->craplog.isBlacklistUsed(
        this->NGINX_ID,
        this->crapview.getLogFieldID( this->ui->box_ConfNginx_Blacklist_Field->currentText() ) );
    this->ui->checkBox_ConfNginx_Blacklist_Used->setChecked( used );
    this->on_checkBox_ConfNginx_Blacklist_Used_clicked( used );
}
void MainWindow::on_checkBox_ConfNginx_Blacklist_Used_clicked(bool checked)
{
    this->craplog.setBlacklistUsed(
        this->NGINX_ID,
        this->crapview.getLogFieldID( this->ui->box_ConfNginx_Blacklist_Field->currentText() ),
        checked );
    if ( checked == true ) {
        this->ui->inLine_ConfNginx_Blacklist_String->setEnabled( true );
        this->ui->list_ConfNginx_Blacklist_List->setEnabled( true );
    } else {
        this->ui->inLine_ConfNginx_Blacklist_String->clear();
        this->ui->inLine_ConfNginx_Blacklist_String->setEnabled( false );
        this->ui->list_ConfNginx_Blacklist_List->clearSelection();
        this->ui->list_ConfNginx_Blacklist_List->setEnabled( false );
    }
}

void MainWindow::on_inLine_ConfNginx_Blacklist_String_cursorPositionChanged(int arg1, int arg2)
{
    if ( arg2 > 0 ) {
        this->ui->button_ConfNginx_Blacklist_Add->setEnabled( true );
    } else {
        this->ui->button_ConfNginx_Blacklist_Add->setEnabled( false );
    }
}
void MainWindow::on_inLine_ConfNginx_Blacklist_String_returnPressed()
{
    this->on_button_ConfNginx_Blacklist_Add_clicked();
}
void MainWindow::on_button_ConfNginx_Blacklist_Add_clicked()
{
    const QString& item = this->ui->inLine_ConfNginx_Blacklist_String->text();
    if ( this->ui->list_ConfNginx_Blacklist_List->findItems( item, Qt::MatchFlag::MatchCaseSensitive ).size() == 0 ) {
        // not in the list yet, append
        this->ui->list_ConfNginx_Blacklist_List->addItem( item );
        this->craplog.blacklistAdd(
            this->NGINX_ID,
            this->crapview.getLogFieldID( this->ui->box_ConfNginx_Blacklist_Field->currentText() ),
            item.toStdString() );
    }
    // select the item in the list, in both cases it was already in or it has been just inserted
    this->ui->list_ConfNginx_Blacklist_List->clearSelection();
    this->ui->list_ConfNginx_Blacklist_List->findItems( item, Qt::MatchFlag::MatchCaseSensitive ).at(0)->setSelected( true );
    this->ui->inLine_ConfNginx_Blacklist_String->clear();
}

void MainWindow::on_list_ConfNginx_Blacklist_List_itemSelectionChanged()
{
    if ( this->ui->list_ConfNginx_Blacklist_List->selectedItems().size() == 1 ) {
        this->ui->button_ConfNginx_Blacklist_Remove->setEnabled( true );
        this->ui->button_ConfNginx_Blacklist_Up->setEnabled( true );
        this->ui->button_ConfNginx_Blacklist_Down->setEnabled( true );
        // polishing
        const auto& item = this->ui->list_ConfNginx_Blacklist_List->selectedItems().at(0);
        const int max = this->ui->list_ConfNginx_Blacklist_List->count() -1;
        if ( max == 0 ) {
            this->ui->button_ConfNginx_Blacklist_Up->setEnabled( false );
            this->ui->button_ConfNginx_Blacklist_Down->setEnabled( false );
        } else {
            for ( int i=0; i<=max; i++ ) {
                if ( this->ui->list_ConfNginx_Blacklist_List->item(i) == item ) {
                    if ( i == 0 ) {
                        this->ui->button_ConfNginx_Blacklist_Up->setEnabled( false );
                    } else if ( i == max ) {
                        this->ui->button_ConfNginx_Blacklist_Down->setEnabled( false );
                    }
                }
            }
        }
    } else {
        this->ui->button_ConfNginx_Blacklist_Remove->setEnabled( false );
        this->ui->button_ConfNginx_Blacklist_Up->setEnabled( false );
        this->ui->button_ConfNginx_Blacklist_Down->setEnabled( false );
    }
}
void MainWindow::on_button_ConfNginx_Blacklist_Remove_clicked()
{
    const auto& item = this->ui->list_ConfNginx_Blacklist_List->selectedItems().at(0);
    this->craplog.blacklistRemove(
        this->NGINX_ID,
        this->crapview.getLogFieldID( this->ui->box_ConfNginx_Blacklist_Field->currentText() ),
        item->text().toStdString() );
    // refresh the list
    this->on_box_ConfNginx_Blacklist_Field_currentTextChanged( this->ui->box_ConfNginx_Blacklist_Field->currentText() );
}
void MainWindow::on_button_ConfNginx_Blacklist_Up_clicked()
{
    const auto& item = this->ui->list_ConfNginx_Blacklist_List->selectedItems().at(0);
    const int i = this->craplog.blacklistMoveUp(
        this->NGINX_ID,
        this->crapview.getLogFieldID( this->ui->box_ConfNginx_Blacklist_Field->currentText() ),
        item->text().toStdString() );
    // refresh the list
    this->on_box_ConfNginx_Blacklist_Field_currentTextChanged( this->ui->box_ConfNginx_Blacklist_Field->currentText() );
    // re-select the item
    this->ui->list_ConfNginx_Blacklist_List->item( i )->setSelected( true );
    this->ui->list_ConfNginx_Blacklist_List->setFocus();
}
void MainWindow::on_button_ConfNginx_Blacklist_Down_clicked()
{
    const auto& item = this->ui->list_ConfNginx_Blacklist_List->selectedItems().at(0);
    const int i = this->craplog.blacklistMoveDown(
        this->NGINX_ID,
        this->crapview.getLogFieldID( this->ui->box_ConfNginx_Blacklist_Field->currentText() ),
        item->text().toStdString() );
    // refresh the list
    this->on_box_ConfNginx_Blacklist_Field_currentTextChanged( this->ui->box_ConfNginx_Blacklist_Field->currentText() );
    // re-select the item
    this->ui->list_ConfNginx_Blacklist_List->item( i )->setSelected( true );
    this->ui->list_ConfNginx_Blacklist_List->setFocus();
}


////////////////
//// IIS ////
// paths
void MainWindow::on_inLine_ConfIis_Path_String_textChanged(const QString &arg1)
{
    if ( arg1.size() > 0 ) {
        std::string path = this->resolvePath( arg1.toStdString() );
        if ( IOutils::checkDir( path ) == true ) {
            this->ui->icon_ConfIis_Path_Wrong->setVisible( false );
            this->ui->button_ConfIis_Path_Save->setEnabled( true );
        } else {
            this->ui->icon_ConfIis_Path_Wrong->setVisible( true );
            this->ui->button_ConfIis_Path_Save->setEnabled( false );
        }
    } else {
        this->ui->icon_ConfIis_Path_Wrong->setVisible( true );
        this->ui->button_ConfIis_Path_Save->setEnabled( false );
    }
}
void MainWindow::on_inLine_ConfIis_Path_String_returnPressed()
{
    this->on_button_ConfIis_Path_Save_clicked();
}
void MainWindow::on_button_ConfIis_Path_Save_clicked()
{
    if ( this->ui->icon_ConfIis_Path_Wrong->isVisible() == false ) {
        // set the paths
        std::string path = this->resolvePath( this->ui->inLine_ConfIis_Path_String->text().toStdString() );
        if ( StringOps::endsWith( path, "/" ) ) {
            path = StringOps::rstrip( path, "/" );
        }
        if ( IOutils::checkDir( path, true ) == false ) {
            DialogSec::warnDirNotReadable( nullptr );
        }
        this->craplog.setLogsPath( this->IIS_ID, path );
        this->ui->inLine_ConfIis_Path_String->setText( QString::fromStdString( path ) );
    }
    this->ui->button_ConfIis_Path_Save->setEnabled( false );
}

// formats
const int MainWindow::getIisLogsModule()
{
    int module = 0;
    if ( this->ui->radio_ConfIis_Format_NCSA->isChecked() == true ) {
        module = 1;
    } else if ( this->ui->radio_ConfIis_Format_IIS->isChecked() == true ) {
        module = 2;
    }
    return module;
}

void MainWindow::on_radio_ConfIis_Format_W3C_toggled(bool checked)
{
    if ( checked == true ) {
        this->craplog.setIisLogFormat( "", 0 );
        this->ui->inLine_ConfIis_Format_String->clear();
        this->ui->inLine_ConfIis_Format_String->setEnabled( true );
        this->ui->inLine_ConfIis_Format_String->setFocus();
    }
}
void MainWindow::on_radio_ConfIis_Format_NCSA_toggled(bool checked)
{
    if ( checked == true ) {
        this->craplog.setIisLogFormat( "c-ip s-sitename s-computername [date:time] sc-status sc-bytes", 1 );
        this->ui->inLine_ConfIis_Format_String->clear();
        this->ui->inLine_ConfIis_Format_String->setText( QString::fromStdString( this->craplog.getLogsFormatString( this->IIS_ID ) ) );
        this->ui->inLine_ConfIis_Format_String->setEnabled( false );
        this->ui->button_ConfIis_Format_Save->setEnabled( false );
    }
}
void MainWindow::on_radio_ConfIis_Format_IIS_toggled(bool checked)
{
    if ( checked == true ) {
        this->craplog.setIisLogFormat( "c-ip, cs-username, date, time, s-sitename, s-computername, s-ip, time-taken, cs-bytes, sc-bytes, sc-status, sc-win32-status, cs-method, cs-uri-stem, cs-uri-query,", 2 );
        this->ui->inLine_ConfIis_Format_String->clear();
        this->ui->inLine_ConfIis_Format_String->setText( QString::fromStdString( this->craplog.getLogsFormatString( this->IIS_ID ) ) );
        this->ui->inLine_ConfIis_Format_String->setEnabled( false );
        this->ui->button_ConfIis_Format_Save->setEnabled( false );
    }
}

void MainWindow::on_inLine_ConfIis_Format_String_cursorPositionChanged(int arg1, int arg2)
{
    if ( arg2 > 0 ) {
        this->ui->button_ConfIis_Format_Save->setEnabled( true );
    } else {
        this->ui->button_ConfIis_Format_Save->setEnabled( false );
    }
}
void MainWindow::on_inLine_ConfIis_Format_String_returnPressed()
{
    if ( this->ui->button_ConfIis_Format_Save->isEnabled() == true ) {
        this->on_button_ConfIis_Format_Save_clicked();
    }
}
void MainWindow::on_button_ConfIis_Format_Save_clicked()
{
    this->craplog.setIisLogFormat( StringOps::strip( this->ui->inLine_ConfIis_Format_String->text().toStdString() ), this->getIisLogsModule() );
    this->ui->button_ConfIis_Format_Save->setEnabled( false );
}
void MainWindow::on_button_ConfIis_Format_Sample_clicked()
{
    this->ui->preview_ConfIis_Format_Sample->setText(
        this->craplog.getLogsFormatSample( this->IIS_ID ) );
}
void MainWindow::on_button_ConfIis_Format_Help_clicked()
{
    this->showHelp( "iis_format" );
}

// warnlists
void MainWindow::on_box_ConfIis_Warnlist_Field_currentTextChanged(const QString &arg1)
{
    this->ui->inLine_ConfIis_Warnlist_String->clear();
    this->ui->list_ConfIis_Warnlist_List->clear();
    // update the list
    const std::vector<std::string>& list = this->craplog.getWarnlist(
        this->IIS_ID, this->crapview.getLogFieldID( arg1 ) );
    for ( const std::string& item : list ) {
        this->ui->list_ConfIis_Warnlist_List->addItem( QString::fromStdString( item ) );
    }
    // check/uncheck the usage option
    bool used = this->craplog.isWarnlistUsed(
        this->IIS_ID,
        this->crapview.getLogFieldID( this->ui->box_ConfIis_Warnlist_Field->currentText() ) );
    this->ui->checkBox_ConfIis_Warnlist_Used->setChecked( used );
    this->on_checkBox_ConfIis_Warnlist_Used_clicked( used );
}
void MainWindow::on_checkBox_ConfIis_Warnlist_Used_clicked(bool checked)
{
    this->craplog.setWarnlistUsed(
        this->IIS_ID,
        this->crapview.getLogFieldID( this->ui->box_ConfIis_Warnlist_Field->currentText() ),
        checked );
    if ( checked == true ) {
        this->ui->inLine_ConfIis_Warnlist_String->setEnabled( true );
        this->ui->list_ConfIis_Warnlist_List->setEnabled( true );
    } else {
        this->ui->inLine_ConfIis_Warnlist_String->clear();
        this->ui->inLine_ConfIis_Warnlist_String->setEnabled( false );
        this->ui->list_ConfIis_Warnlist_List->clearSelection();
        this->ui->list_ConfIis_Warnlist_List->setEnabled( false );
    }
}

void MainWindow::on_inLine_ConfIis_Warnlist_String_cursorPositionChanged(int arg1, int arg2)
{
    if ( arg2 > 0 ) {
        this->ui->button_ConfIis_Warnlist_Add->setEnabled( true );
    } else {
        this->ui->button_ConfIis_Warnlist_Add->setEnabled( false );
    }
}
void MainWindow::on_inLine_ConfIis_Warnlist_String_returnPressed()
{
    this->on_button_ConfIis_Warnlist_Add_clicked();
}
void MainWindow::on_button_ConfIis_Warnlist_Add_clicked()
{
    const QString& item = this->ui->inLine_ConfIis_Warnlist_String->text();
    if ( this->ui->list_ConfIis_Warnlist_List->findItems( item, Qt::MatchFlag::MatchCaseSensitive ).size() == 0 ) {
        // not in the list yet, append
        this->ui->list_ConfIis_Warnlist_List->addItem( item );
        this->craplog.warnlistAdd(
            this->IIS_ID,
            this->crapview.getLogFieldID( this->ui->box_ConfIis_Warnlist_Field->currentText() ),
            item.toStdString() );
    }
    // select the item in the list, in both cases it was already in or it has been just inserted
    this->ui->list_ConfIis_Warnlist_List->clearSelection();
    this->ui->list_ConfIis_Warnlist_List->findItems( item, Qt::MatchFlag::MatchCaseSensitive ).at(0)->setSelected( true );
    this->ui->inLine_ConfIis_Warnlist_String->clear();
}

void MainWindow::on_list_ConfIis_Warnlist_List_itemSelectionChanged()
{
    if ( this->ui->list_ConfIis_Warnlist_List->selectedItems().size() == 1 ) {
        this->ui->button_ConfIis_Warnlist_Remove->setEnabled( true );
        this->ui->button_ConfIis_Warnlist_Up->setEnabled( true );
        this->ui->button_ConfIis_Warnlist_Down->setEnabled( true );
        // polishing
        const auto& item = this->ui->list_ConfIis_Warnlist_List->selectedItems().at(0);
        const int max = this->ui->list_ConfIis_Warnlist_List->count() -1;
        if ( max == 0 ) {
            this->ui->button_ConfIis_Warnlist_Up->setEnabled( false );
            this->ui->button_ConfIis_Warnlist_Down->setEnabled( false );
        } else {
            for ( int i=0; i<=max; i++ ) {
                if ( this->ui->list_ConfIis_Warnlist_List->item(i) == item ) {
                    if ( i == 0 ) {
                        this->ui->button_ConfIis_Warnlist_Up->setEnabled( false );
                    } else if ( i == max ) {
                        this->ui->button_ConfIis_Warnlist_Down->setEnabled( false );
                    }
                }
            }
        }
    } else {
        this->ui->button_ConfIis_Warnlist_Remove->setEnabled( false );
        this->ui->button_ConfIis_Warnlist_Up->setEnabled( false );
        this->ui->button_ConfIis_Warnlist_Down->setEnabled( false );
    }
}
void MainWindow::on_button_ConfIis_Warnlist_Remove_clicked()
{
    const auto& item = this->ui->list_ConfIis_Warnlist_List->selectedItems().at(0);
    this->craplog.warnlistRemove(
        this->IIS_ID,
        this->crapview.getLogFieldID( this->ui->box_ConfIis_Warnlist_Field->currentText() ),
        item->text().toStdString() );
    // refresh the list
    this->on_box_ConfIis_Warnlist_Field_currentTextChanged( this->ui->box_ConfIis_Warnlist_Field->currentText() );
}
void MainWindow::on_button_ConfIis_Warnlist_Up_clicked()
{
    const auto& item = this->ui->list_ConfIis_Warnlist_List->selectedItems().at(0);
    const int i = this->craplog.warnlistMoveUp(
        this->IIS_ID,
        this->crapview.getLogFieldID( this->ui->box_ConfIis_Warnlist_Field->currentText() ),
        item->text().toStdString() );
    // refresh the list
    this->on_box_ConfIis_Warnlist_Field_currentTextChanged( this->ui->box_ConfIis_Warnlist_Field->currentText() );
    // re-select the item
    this->ui->list_ConfIis_Warnlist_List->item( i )->setSelected( true );
    this->ui->list_ConfIis_Warnlist_List->setFocus();
}
void MainWindow::on_button_ConfIis_Warnlist_Down_clicked()
{
    const auto& item = this->ui->list_ConfIis_Warnlist_List->selectedItems().at(0);
    const int i = this->craplog.warnlistMoveDown(
        this->IIS_ID,
        this->crapview.getLogFieldID( this->ui->box_ConfIis_Warnlist_Field->currentText() ),
        item->text().toStdString() );
    // refresh the list
    this->on_box_ConfIis_Warnlist_Field_currentTextChanged( this->ui->box_ConfIis_Warnlist_Field->currentText() );
    // re-select the item
    this->ui->list_ConfIis_Warnlist_List->item( i )->setSelected( true );
    this->ui->list_ConfIis_Warnlist_List->setFocus();
}


// blacklist
void MainWindow::on_box_ConfIis_Blacklist_Field_currentTextChanged(const QString &arg1)
{
    this->ui->inLine_ConfIis_Blacklist_String->clear();
    this->ui->list_ConfIis_Blacklist_List->clear();
    // update the list
    const std::vector<std::string>& list = this->craplog.getBlacklist(
        this->IIS_ID, this->crapview.getLogFieldID( arg1 ) );
    for ( const std::string& item : list ) {
        this->ui->list_ConfIis_Blacklist_List->addItem( QString::fromStdString( item ) );
    }
    // check/uncheck the usage option
    bool used = this->craplog.isBlacklistUsed(
        this->IIS_ID,
        this->crapview.getLogFieldID( this->ui->box_ConfIis_Blacklist_Field->currentText() ) );
    this->ui->checkBox_ConfIis_Blacklist_Used->setChecked( used );
    this->on_checkBox_ConfIis_Blacklist_Used_clicked( used );
}
void MainWindow::on_checkBox_ConfIis_Blacklist_Used_clicked(bool checked)
{
    this->craplog.setBlacklistUsed(
        this->IIS_ID,
        this->crapview.getLogFieldID( this->ui->box_ConfIis_Blacklist_Field->currentText() ),
        checked );
    if ( checked == true ) {
        this->ui->inLine_ConfIis_Blacklist_String->setEnabled( true );
        this->ui->list_ConfIis_Blacklist_List->setEnabled( true );
    } else {
        this->ui->inLine_ConfIis_Blacklist_String->clear();
        this->ui->inLine_ConfIis_Blacklist_String->setEnabled( false );
        this->ui->list_ConfIis_Blacklist_List->clearSelection();
        this->ui->list_ConfIis_Blacklist_List->setEnabled( false );
    }
}

void MainWindow::on_inLine_ConfIis_Blacklist_String_cursorPositionChanged(int arg1, int arg2)
{
    if ( arg2 > 0 ) {
        this->ui->button_ConfIis_Blacklist_Add->setEnabled( true );
    } else {
        this->ui->button_ConfIis_Blacklist_Add->setEnabled( false );
    }
}
void MainWindow::on_inLine_ConfIis_Blacklist_String_returnPressed()
{
    this->on_button_ConfIis_Blacklist_Add_clicked();
}
void MainWindow::on_button_ConfIis_Blacklist_Add_clicked()
{
    const QString& item = this->ui->inLine_ConfIis_Blacklist_String->text();
    if ( this->ui->list_ConfIis_Blacklist_List->findItems( item, Qt::MatchFlag::MatchCaseSensitive ).size() == 0 ) {
        // not in the list yet, append
        this->ui->list_ConfIis_Blacklist_List->addItem( item );
        this->craplog.blacklistAdd(
            this->IIS_ID,
            this->crapview.getLogFieldID( this->ui->box_ConfIis_Blacklist_Field->currentText() ),
            item.toStdString() );
    }
    // select the item in the list, in both cases it was already in or it has been just inserted
    this->ui->list_ConfIis_Blacklist_List->clearSelection();
    this->ui->list_ConfIis_Blacklist_List->findItems( item, Qt::MatchFlag::MatchCaseSensitive ).at(0)->setSelected( true );
    this->ui->inLine_ConfIis_Blacklist_String->clear();
}

void MainWindow::on_list_ConfIis_Blacklist_List_itemSelectionChanged()
{
    if ( this->ui->list_ConfIis_Blacklist_List->selectedItems().size() == 1 ) {
        this->ui->button_ConfIis_Blacklist_Remove->setEnabled( true );
        this->ui->button_ConfIis_Blacklist_Up->setEnabled( true );
        this->ui->button_ConfIis_Blacklist_Down->setEnabled( true );
        // polishing
        const auto& item = this->ui->list_ConfIis_Blacklist_List->selectedItems().at(0);
        const int max = this->ui->list_ConfIis_Blacklist_List->count() -1;
        if ( max == 0 ) {
            this->ui->button_ConfIis_Blacklist_Up->setEnabled( false );
            this->ui->button_ConfIis_Blacklist_Down->setEnabled( false );
        } else {
            for ( int i=0; i<=max; i++ ) {
                if ( this->ui->list_ConfIis_Blacklist_List->item(i) == item ) {
                    if ( i == 0 ) {
                        this->ui->button_ConfIis_Blacklist_Up->setEnabled( false );
                    } else if ( i == max ) {
                        this->ui->button_ConfIis_Blacklist_Down->setEnabled( false );
                    }
                }
            }
        }
    } else {
        this->ui->button_ConfIis_Blacklist_Remove->setEnabled( false );
        this->ui->button_ConfIis_Blacklist_Up->setEnabled( false );
        this->ui->button_ConfIis_Blacklist_Down->setEnabled( false );
    }
}
void MainWindow::on_button_ConfIis_Blacklist_Remove_clicked()
{
    const auto& item = this->ui->list_ConfIis_Blacklist_List->selectedItems().at(0);
    this->craplog.blacklistRemove(
        this->IIS_ID,
        this->crapview.getLogFieldID( this->ui->box_ConfIis_Blacklist_Field->currentText() ),
        item->text().toStdString() );
    // refresh the list
    this->on_box_ConfIis_Blacklist_Field_currentTextChanged( this->ui->box_ConfIis_Blacklist_Field->currentText() );
}
void MainWindow::on_button_ConfIis_Blacklist_Up_clicked()
{
    const auto& item = this->ui->list_ConfIis_Blacklist_List->selectedItems().at(0);
    const int i = this->craplog.blacklistMoveUp(
        this->IIS_ID,
        this->crapview.getLogFieldID( this->ui->box_ConfIis_Blacklist_Field->currentText() ),
        item->text().toStdString() );
    // refresh the list
    this->on_box_ConfIis_Blacklist_Field_currentTextChanged( this->ui->box_ConfIis_Blacklist_Field->currentText() );
    // re-select the item
    this->ui->list_ConfIis_Blacklist_List->item( i )->setSelected( true );
    this->ui->list_ConfIis_Blacklist_List->setFocus();
}
void MainWindow::on_button_ConfIis_Blacklist_Down_clicked()
{
    const auto& item = this->ui->list_ConfIis_Blacklist_List->selectedItems().at(0);
    const int i = this->craplog.blacklistMoveDown(
        this->IIS_ID,
        this->crapview.getLogFieldID( this->ui->box_ConfIis_Blacklist_Field->currentText() ),
        item->text().toStdString() );
    // refresh the list
    this->on_box_ConfIis_Blacklist_Field_currentTextChanged( this->ui->box_ConfIis_Blacklist_Field->currentText() );
    // re-select the item
    this->ui->list_ConfIis_Blacklist_List->item( i )->setSelected( true );
    this->ui->list_ConfIis_Blacklist_List->setFocus();
}



