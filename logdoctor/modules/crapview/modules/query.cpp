
#include "query.h"

#include "modules/dialogs.h"
#include "modules/exceptions.h"
#include "utilities/strings.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDateTime>
#include <QDate>


DbQuery::DbQuery()
{

}

void DbQuery::setDialogLevel(const int& new_level )
{
    this->dialog_level = new_level;
}

void DbQuery::setDbPath( const std::string& path )
{
    this->db_path = path;
    this->db_name = QString::fromStdString( this->db_path.substr( this->db_path.find_last_of( '/' ) + 1 ) );
}


const int DbQuery::getMinuteGap( const int& minute , const int& gap )
{
    int m = -1;
    if ( minute < 0 || minute >= 60 ) {
        // unexpected value
        throw DateTimeException( "Unexpected Minute: "+std::to_string( minute ) );
    }
    int n = 0;
    for ( int g=0; g<60; g+=gap ) {
        if ( minute >= g && minute < g+gap ) {
            m = gap * n;
            break;
        }
        n++;
    }
    return m;
}

const int DbQuery::getMonthDays( const int& year, const int& month )
{
    int n_days;
    switch (month) {
        case 1:  n_days = 31; break;
        case 2:  n_days = ( year%4 == 0 ) ? 29 : 28 ; break;
        case 3:  n_days = 31; break;
        case 4:  n_days = 30; break;
        case 5:  n_days = 31; break;
        case 6:  n_days = 30; break;
        case 7:  n_days = 31; break;
        case 8:  n_days = 31; break;
        case 9:  n_days = 30; break;
        case 10: n_days = 31; break;
        case 11: n_days = 30; break;
        case 12: n_days = 31; break;
        default:
            // unexpected month
            throw DateTimeException( "Unexpected Month: "+std::to_string( month ) );
    }
    return n_days;
}

const int DbQuery::getMonthsCount( const QString& from_year, const QString& from_month, const QString& to_year, const QString& to_month )
{
    int from_year_, from_month_, to_year_, to_month_;
    try {
        from_year_  = from_year.toInt();
        from_month_ = this->getMonthNumber( from_month );
        to_year_  = ( to_year.size() == 0 )  ? from_year_  : to_year.toInt() ;
        to_month_ = ( to_month.size() == 0 ) ? from_month_ : this->getMonthNumber( to_month ) ;
    } catch (...) {
        // failed to convert to integers
        throw DateTimeException( "Failed to convert Month from string to int" ); // leave un-catched
    }
    return this->getMonthsCount( from_year_, from_month_, to_year_, to_month_ );
}

const int DbQuery::getMonthsCount( const int& from_year, const int& from_month, const int& to_year, const int& to_month )
{
    int n_months = 0;
    if ( from_year == to_year ) {
        // same year
        if ( from_month == to_month ) {
            // same month
            n_months = 1;
        } else {
            // different months
            n_months = to_month - from_month + 1;
        }
    } else {
        // different years
        n_months += 13 - from_month; // months to the end of the first year
        n_months += to_month; // months from the beginning of the last year
        n_months += 12 * ( to_year - from_year - 1 ); // 12 months for every middle year (0 if none)
    }
    return n_months;
}


const int DbQuery::getMonthNumber( const QString& month_str )
{
    int m=0;
    for ( const auto& [num,str] : this->MONTHS ) {
        if ( TR::tr(str.c_str()) == month_str ) {
            m = num;
            break;
        }
    }
    return m;
}


/*const int DbQuery::getLogFieldID ( const QString& field_str )
{
    int f=0;
    for ( const auto& [id,str] : this->MONTHS ) {
        if ( TR::tr(str.c_str()) == field_str ) {
            f = id;
            break;
        }
    }
    return f;
}*/

const QString DbQuery::getDbField( const QString& tr_fld )
{
    QString f;
    for ( const auto& [id,str] : this->FIELDS ) {
        if ( TR::tr(str.c_str()) == tr_fld ) {
            f = this->LogFields_to_DbFields.at( str );
            break;
        }
    }
    return f;
}



// get a fresh map of available dates
void DbQuery::refreshDates(std::tuple<bool, std::unordered_map<int, std::unordered_map<int, std::unordered_map<int, std::vector<int>>>>> &result)
{
    bool successful = true;
    std::unordered_map<int, std::unordered_map<int, std::unordered_map<int, std::vector<int>>>> dates = {
        {11, {}}, {12, {}}, {13, {}} };

    QSqlDatabase db;
    if ( QSqlDatabase::contains("qt_sql_default_connection") ) {
        db = QSqlDatabase::database("qt_sql_default_connection");
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE");
    }
    db.setDatabaseName( QString::fromStdString( this->db_path ));

    if ( ! db.open() ) {
        // error opening database
        successful = false;
        QString err_msg = "";
        if ( this->dialog_level == 2 ) {
            err_msg = db.lastError().text();
        }
        DialogSec::errDatabaseFailedOpening( this->db_name, err_msg );

    } else {
        // recursively query years, months and days for every WebServer
        std::vector<std::tuple<int, QString>> tables = {
            std::make_tuple(11,"apache"),
            std::make_tuple(12,"nginx"),
            std::make_tuple(13,"iis") };

        QSqlQuery Y_query = QSqlQuery( db ),
                  M_query = QSqlQuery( db ),
                  D_query = QSqlQuery( db );

        for ( const auto& table : tables ) {
            if ( ! successful ) { break; }

            int ws = std::get<0>(table);
            QString tbl = std::get<1>(table);

            if ( ! Y_query.exec( QString("SELECT DISTINCT \"year\" FROM \"%1\" ORDER BY \"year\" ASC;").arg(tbl) ) ) {
                // error querying database
                successful = false;
                DialogSec::errDatabaseFailedExecuting( this->db_name, Y_query.lastQuery(), Y_query.lastError().text() );
                break;

            } else {
                int year, month, day;
                while ( Y_query.next() ) {
                    try {
                        year = Y_query.value(0).toInt();
                    } catch (...) {
                        // failed to convert to integer
                        successful = false;
                        QString err_msg = TR::tr(this->MSG_ERR_PARSING_YMD.c_str()).arg( TR::tr(this->WORD_YEARS.c_str()) );
                        if ( this->dialog_level > 0 ) {
                            err_msg += QString("\n\n%1:\n%2").arg( TR::tr(this->MSG_RESPONSIBLE_VALUE.c_str()), Y_query.value(0).toString() );
                            if ( this->dialog_level == 2 ) {
                                err_msg += QString("\n\n%1:\n%2").arg( TR::tr(this->MSG_TABLE_NAME.c_str()), tbl );
                            }
                        }
                        DialogSec::errGeneric( err_msg );
                        break;
                    }
                    // successfully get the year
                    dates.at( ws ).emplace( year, std::unordered_map<int, std::vector<int>>() );
                    // query any available month
                    if ( ! M_query.exec( QString("SELECT DISTINCT \"month\" FROM \"%1\" WHERE \"year\"=%2 ORDER BY \"month\" ASC;").arg(tbl).arg(year) ) ) {
                        // error querying database
                        successful = false;
                        DialogSec::errDatabaseFailedExecuting( this->db_name, M_query.lastQuery(), M_query.lastError().text() );
                        break;

                    } else {
                        while ( M_query.next() ) {
                            try {
                                month = M_query.value(0).toInt();
                            } catch (...) {
                                // failed to convert to integer
                                successful = false;
                                QString err_msg = TR::tr(this->MSG_ERR_PARSING_YMD.c_str()).arg( TR::tr(this->WORD_MONTHS.c_str()) );
                                if ( this->dialog_level > 0 ) {
                                    err_msg += QString("\n\n%1:\n%2").arg( TR::tr(this->MSG_RESPONSIBLE_VALUE.c_str()), M_query.value(0).toString() );
                                    if ( this->dialog_level == 2 ) {
                                        err_msg += QString("\n\n%1:\n%2").arg( TR::tr(this->MSG_TABLE_NAME.c_str()), tbl );
                                    }
                                }
                                DialogSec::errGeneric( err_msg );
                                break;
                            }
                            // successfully get the month
                            dates.at( ws ).at( year ).emplace( month, std::vector<int>() );
                            // query any available day
                            if ( ! D_query.exec( QString("SELECT DISTINCT \"day\" FROM \"%1\" WHERE \"year\"=%2 AND \"month\"=%3 ORDER BY \"day\" ASC;").arg(tbl).arg(year).arg(month) ) ) {
                                // error querying database
                                successful = false;
                                DialogSec::errDatabaseFailedExecuting( this->db_name, D_query.lastQuery(), D_query.lastError().text() );
                                break;

                            } else {
                                while ( D_query.next() ) {
                                    try {
                                        day = D_query.value(0).toInt();
                                    } catch (...) {
                                        // failed to convert to integer
                                        successful = false;
                                        QString err_msg = TR::tr(this->MSG_ERR_PARSING_YMD.c_str()).arg( TR::tr(this->WORD_DAYS.c_str()) );
                                        if ( this->dialog_level > 0 ) {
                                            err_msg += QString("\n\n%1:\n%2").arg( TR::tr(this->MSG_RESPONSIBLE_VALUE.c_str()), D_query.value(0).toString() );
                                            if ( this->dialog_level == 2 ) {
                                                err_msg += QString("\n\n%1:\n%2").arg( TR::tr(this->MSG_TABLE_NAME.c_str()), tbl );
                                            }
                                        }
                                        DialogSec::errGeneric( err_msg );
                                        break;
                                    }
                                    // successfully get the day
                                    dates.at( ws ).at( year ).at( month ).push_back( day );
                                }
                                D_query.finish();
                                // break if something went wrong
                                if ( ! successful ) { break; }
                            }
                        }
                        M_query.finish();
                        // break if something went wrong
                        if ( ! successful ) { break; }
                    }
                }
                Y_query.finish();
                // break if something went wrong
                if ( ! successful ) { break; }
            }
        }
    }

    if ( ! successful ) {
        dates.clear();
    }
    if ( db.isOpen() ) {
        db.close();
    }
    result = std::make_tuple( successful, dates );
}


// update the values for the warnings
void DbQuery::updateWarnings( const QString& web_server, const std::vector<std::tuple<int, int>>& updates )
{
    bool successful = true;

    QSqlDatabase db;
    if ( QSqlDatabase::contains("qt_sql_default_connection") ) {
        db = QSqlDatabase::database("qt_sql_default_connection");
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE");
    }
    db.setDatabaseName( QString::fromStdString( this->db_path ));

    if ( ! db.open() ) {
        // error opening database
        QString err_msg = "";
        if ( this->dialog_level == 2 ) {
            err_msg = db.lastError().text();
        }
        DialogSec::errDatabaseFailedOpening( this->db_name, err_msg );

    } else {
        if ( web_server != "apache" && web_server != "nginx" && web_server != "iis" ) {
            // unexpected WebServer
            successful = false;
            DialogSec::errGeneric( QString("%1:\n%2").arg( TR::tr(this->MSG_ERR_UNX_WS.c_str()), web_server ), true );
        }

        if ( successful ) {
            // update the database
            QSqlQuery query = QSqlQuery( db );

            for ( const auto& data : updates ) {
                // build the query statement
                QString stmt = QString("UPDATE \"%1\" SET warning=%2 WHERE rowid=%3;")
                        .arg( web_server )
                        .arg( std::get<1>(data) )
                        .arg( std::get<0>(data) );

                if ( ! query.exec( stmt.replace("'","''") ) ) {
                    // error querying database
                    DialogSec::errDatabaseFailedExecuting( this->db_name, query.lastQuery(), query.lastError().text() );
                    break;
                }
            }
        }
    }
    if ( db.isOpen() ) {
        db.close();
    }
}


// get daytime values for the warnings
void DbQuery::getWarnCounts( std::tuple<bool, std::vector<std::vector<std::vector<std::vector<QString>>>>> &result, const QString& web_server, const QString& year_, const QString& month_, const QString& day_, const QString& hour_ )
{
    bool successful = true;
    std::vector<std::vector<std::vector<std::vector<QString>>>> items;

    QSqlDatabase db;
    if ( QSqlDatabase::contains("qt_sql_default_connection") ) {
        db = QSqlDatabase::database("qt_sql_default_connection");
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE");
    }
    db.setDatabaseName( QString::fromStdString( this->db_path ));

    if ( ! db.open() ) {
        // error opening database
        successful = false;
        QString err_msg = "";
        if ( this->dialog_level == 2 ) {
            err_msg = db.lastError().text();
        }
        DialogSec::errDatabaseFailedOpening( this->db_name, err_msg );

    } else {
        if ( web_server != "apache" && web_server != "nginx" && web_server != "iis" ) {
            // unexpected WebServer
            successful = false;
            DialogSec::errGeneric( QString("%1:\n%2").arg( TR::tr(this->MSG_ERR_UNX_WS.c_str()), web_server ), true );
        }
        int year, month, day, hour;
        if ( successful ) {
            // setup period limits
            try {
                year  = year_.toInt();
                month = this->getMonthNumber( month_ );
                day   = day_.toInt();
                if ( hour_.size() > 0 ) {
                    hour = hour_.toInt();
                }
            } catch (...) {
                // failed to convert to integers
                successful = false;
                DialogSec::errGeneric( TR::tr(this->MSG_ERR_PROCESSING_DATES.c_str()), true );
            }
        }
        if ( successful ) {
            // build the query statement
            QSqlQuery query = QSqlQuery( db );
            QString stmt = QString("SELECT rowid, * FROM \"%1\" WHERE \"year\"=%2 AND \"month\"=%3 AND \"day\"=%4")
                    .arg( web_server )
                    .arg( year ).arg( month ).arg( day );

            if ( hour_.size() == 0 ) {
                // entire day
                for ( int h=0; h<24; h++ ) {
                    items.push_back( std::vector<std::vector<std::vector<QString>>>() );
                    std::vector<std::vector<std::vector<QString>>>& aux = items.at( h );
                    for ( int m=0; m<60; m+=10 ) {
                        aux.push_back( std::vector<std::vector<QString>>() );
                    }
                }

                stmt += " ORDER BY \"hour\",\"minute\",\"second\" ASC;";
                if ( ! query.exec( stmt.replace("'","''") ) ) {
                    // error querying database
                    successful = false;
                    DialogSec::errDatabaseFailedExecuting( this->db_name, query.lastQuery(), query.lastError().text() );

                } else {
                    try {
                        // get query data
                        while ( query.next() ) {
                            std::vector<QString> aux;
                            for ( int i=1; i<13; i++ ) {
                                aux.push_back( query.value( i ).toString() );
                            }
                            for ( int i=19; i>12; i-- ) {
                                aux.push_back( query.value( i ).toString() );
                            }
                            aux.push_back( query.value( 0 ).toString() );
                            // append the line
                            items.at( query.value(5).toInt() )
                                 .at( this->getMinuteGap( query.value(6).toInt() )/10 )
                                 .push_back( aux );
                        }
                    } catch (...) {
                        // something failed
                        successful = false;
                        DialogSec::errGeneric( TR::tr(this->MSG_ERR_PROCESSING.c_str()), true );
                    }
                }

            } else {
                // 1 hour
                for ( int g=0; g<6; g++ ) {
                    items.push_back( std::vector<std::vector<std::vector<QString>>>() );
                    std::vector<std::vector<std::vector<QString>>>& aux = items.at( g );
                    for ( int m=0; m<10; m++ ) {
                        aux.push_back( std::vector<std::vector<QString>>() );
                    }
                }

                stmt += QString(" AND \"hour\"=%5 ORDER BY \"minute\",\"second\" ASC;").arg( hour );
                if ( ! query.exec( stmt.replace("'","''") ) ) {
                    // error querying database
                    successful = false;
                    DialogSec::errDatabaseFailedExecuting( this->db_name, query.lastQuery(), query.lastError().text() );

                } else {
                    try {
                        // get query data
                        while ( query.next() ) {
                            std::vector<QString> aux;
                            for ( int i=1; i<13; i++ ) {
                                aux.push_back( query.value( i ).toString() );
                            }
                            for ( int i=19; i>12; i-- ) {
                                aux.push_back( query.value( i ).toString() );
                            }
                            aux.push_back( query.value( 0 ).toString() );
                            // append the line
                            items.at( this->getMinuteGap( query.value(6).toInt() )/10 )
                                 .at( query.value(6).toInt()%10 )
                                 .push_back( aux );
                        }
                    } catch (...) {
                        // something failed
                        successful = false;
                        DialogSec::errGeneric( TR::tr(this->MSG_ERR_PROCESSING.c_str()), true );
                    }
                }
            }
        }
    }
    if ( ! successful ) {
        items.clear();
    }
    if ( db.isOpen() ) {
        db.close();
    }
    result = std::make_tuple( successful, items );
}


// get day-time values for the time-taken field
void DbQuery::getSpeedData(std::tuple<bool, std::vector<std::tuple<long long, std::vector<QString>>>>& result, const QString& web_server, const QString& year_, const QString& month_, const QString& day_, const QString& protocol_f, const QString& method_f, const QString& uri_f, const QString& query_f, const QString& response_f )
{
    bool successful = true;
    std::vector<std::tuple<long long, std::vector<QString>>> data;

    QSqlDatabase db;
    if ( QSqlDatabase::contains("qt_sql_default_connection") ) {
        db = QSqlDatabase::database("qt_sql_default_connection");
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE");
    }
    db.setDatabaseName( QString::fromStdString( this->db_path ));

    if ( ! db.open() ) {
        // error opening database
        successful = false;
        QString err_msg = "";
        if ( this->dialog_level == 2 ) {
            err_msg = db.lastError().text();
        }
        DialogSec::errDatabaseFailedOpening( this->db_name, err_msg );

    } else {
        if ( web_server != "apache" && web_server != "nginx" && web_server != "iis" ) {
            // unexpected WebServer
            successful = false;
            DialogSec::errGeneric( QString("%1:\n%2").arg( TR::tr(this->MSG_ERR_UNX_WS.c_str()), web_server ), true );
        }
        int year, month, day;
        if ( successful ) {
            // setup period limits
            try {
                year  = year_.toInt();
                month = this->getMonthNumber( month_ );
                day   = day_.toInt();
            } catch (...) {
                // failed to convert to integers
                successful = false;
                DialogSec::errGeneric( TR::tr(this->MSG_ERR_PROCESSING_DATES.c_str()), true );
            }
        }
        if ( successful ) {
            QDateTime time;
            time.setDate( QDate( year, month , day ) );
            // build the query statement
            QSqlQuery query = QSqlQuery( db );
            QString stmt;

            // prepare the statement
            stmt = QString("SELECT \"hour\",\"minute\",\"second\",\"time_taken\",\"uri\",\"query\",\"method\",\"protocol\",\"response\" FROM \"%1\" WHERE \"year\"=%2 AND \"month\"=%3 AND \"day\"=%4 AND \"time_taken\" IS NOT NULL")
                .arg( web_server )
                .arg( year ).arg( month ).arg( day );

            // apply a filter if present
            if ( protocol_f.size() > 0 ) {
                if ( protocol_f == "NULL" ) {
                    // only select NULL values
                    stmt += QString(" AND \"protocol\" IS NULL");
                } else if ( protocol_f == "NOT NULL" ) {
                    // only select non-NULL values
                    stmt += QString(" AND \"protocol\" IS NOT NULL");
                } else {
                    stmt += QString(" AND \"protocol\" LIKE '%1' || '%'")
                        .arg( QString(protocol_f).replace("'","''") );
                }
            }
            if ( method_f.size() > 0 ) {
                if ( method_f == "NULL" ) {
                    // only select NULL values
                    stmt += QString(" AND \"method\" IS NULL");
                } else if ( method_f == "NOT NULL" ) {
                    // only select non-NULL values
                    stmt += QString(" AND \"method\" IS NOT NULL");
                } else {
                    stmt += QString(" AND \"method\" LIKE '%1' || '%'")
                        .arg( QString(method_f).replace("'","''") );
                }
            }
            if ( uri_f.size() > 0 ) {
                if ( uri_f == "NULL" ) {
                    // only select NULL values
                    stmt += QString(" AND \"uri\" IS NULL");
                } else if ( uri_f == "NOT NULL" ) {
                    // only select non-NULL values
                    stmt += QString(" AND \"uri\" IS NOT NULL");
                } else {
                    stmt += QString(" AND \"uri\" LIKE '%1' || '%'")
                        .arg( QString(uri_f).replace("'","''") );
                }
            }
            if ( query_f.size() > 0 ) {
                if ( query_f == "NULL" ) {
                    // only select NULL values
                    stmt += QString(" AND \"query\" IS NULL");
                } else if ( query_f == "NOT NULL" ) {
                    // only select non-NULL values
                    stmt += QString(" AND \"query\" IS NOT NULL");
                } else {
                    stmt += QString(" AND \"query\" LIKE '%1' || '%'")
                        .arg( QString(query_f).replace("'","''") );
                }
            }
            if ( response_f.size() > 0 ) {
                // numbers
                if ( response_f == "NULL" ) {
                    // only select NULL values
                    stmt += QString(" AND \"response\" IS NULL");
                } else if ( response_f == "NOT NULL" ) {
                    // only select non-NULL values
                    stmt += QString(" AND \"response\" IS NOT NULL");
                } else {
                    QString filter;
                    if ( StringOps::isNumeric( response_f.toStdString() ) ) {
                        // no operator found, set defult to '='
                        filter = QString("=%1").arg( response_f );
                    } else {
                        filter = response_f;
                    }
                    stmt += QString(" AND \"response\"%1")
                        .arg( filter.replace("'","''") );
                }
            }

            stmt += QString(" ORDER BY \"hour\",\"minute\",\"second\" ASC;");
            if ( ! query.exec( stmt ) ) {
                // error querying database
                successful = false;
                DialogSec::errDatabaseFailedExecuting( this->db_name, query.lastQuery(), query.lastError().text() );

            } else {
                try {

                    if ( query.size() == 0 ) {
                        // no result found
                        ;

                    } else {
                        // get query data
                        int hour=-1,  aux_hour,   prev_hour=0,   h,
                            minute=0, aux_minute, prev_minute=0, m,
                            second=0, aux_second, prev_second=0, s;
                        QString tt, ur, qr, mt, pt, rs;
                        while ( query.next() ) {
                            aux_hour   = query.value(0).toInt();
                            aux_minute = query.value(1).toInt();
                            aux_second = query.value(2).toInt();

                            if ( aux_hour == hour && aux_minute == minute && aux_second == second ) {
                                time.setTime( QTime( hour, minute, second ));
                                data.push_back( std::make_tuple(
                                    time.toMSecsSinceEpoch(),
                                    std::vector<QString>{tt,ur,qr,mt,pt,rs} ));
                            } else {
                                if ( aux_hour == hour ) {
                                    h=hour; m=minute; s=second-1;
                                    if ( s < 0 ) {
                                        s=59; m--;
                                        if ( m < 0 ) {
                                            m=59; h--;
                                            if ( h < 0 ) {
                                                h=m=s=0;
                                            }
                                        }
                                    }
                                    // append the second before the last one found, if it is not equal to the prev
                                    if ( prev_hour < h || prev_minute < m || prev_second < s ) {
                                        time.setTime( QTime( h, m, s ));
                                        data.push_back( std::make_tuple(
                                            time.toMSecsSinceEpoch(),
                                            std::vector<QString>{"","","","","",""} ));
                                    }
                                    // same hour new minute/second, append the last count
                                    time.setTime( QTime( hour, minute, second ));
                                    data.push_back( std::make_tuple(
                                        time.toMSecsSinceEpoch(),
                                        std::vector<QString>{tt,ur,qr,mt,pt,rs} ));
                                    // append the second after the last one found, if it is not equal to the next
                                    h=hour; m=minute; s=second+1;
                                    if ( s > 59 ) {
                                        s=0; m++;
                                        if ( m > 59 ) {
                                            m=0; h++;
                                            if ( h > 23 ) {
                                                h=23;m=59;s=59;
                                            }
                                        }
                                    }
                                    if ( aux_hour > h || aux_minute > m || aux_second > s ) {
                                        time.setTime( QTime( h, m, s ));
                                        data.push_back( std::make_tuple(
                                            time.toMSecsSinceEpoch(),
                                            std::vector<QString>{"","","","","",""} ));
                                    }
                                    prev_hour = hour; // update now to avoid getting aux_hour's value
                                } else {
                                    // minute & second are always different when the hour is different
                                    if ( hour >= 0 ) {
                                        // here only in the first round of the loop
                                        // append the prev as zero
                                        h=hour; m=minute; s=second-1;
                                        if ( s < 0 ) {
                                            s=59; m--;
                                            if ( m < 0 ) {
                                                m=59; h--;
                                                if ( h < 0 ) {
                                                    h=m=s=0;
                                                }
                                            }
                                        }
                                        if ( prev_hour < h || prev_minute < m || prev_second < s ) {
                                            time.setTime( QTime( h, m, s ));
                                            data.push_back( std::make_tuple(
                                                time.toMSecsSinceEpoch(),
                                                std::vector<QString>{"","","","","",""} ));
                                        }
                                        // apend the last p count if not in the first round of the loop
                                        time.setTime( QTime( hour, minute, second ));
                                        data.push_back( std::make_tuple(
                                            time.toMSecsSinceEpoch(),
                                            std::vector<QString>{tt,ur,qr,mt,pt,rs} ));
                                        // append the next as zero
                                        h=hour; m=minute; s=second+1;
                                        if ( s > 59 ) {
                                            s=0; m++;
                                            if ( m > 59 ) {
                                                m=0; h++;
                                                if ( h > 23 ) {
                                                    h=23;m=59;s=59;
                                                }
                                            }
                                        }
                                        if ( aux_hour > h || aux_minute > m || aux_second > s ) {
                                            time.setTime( QTime( h, m, s ));
                                            data.push_back( std::make_tuple(
                                                time.toMSecsSinceEpoch(),
                                                std::vector<QString>{"","","","","",""} ));
                                        }
                                    } else {
                                        // append the second 0 of the day, if it is not the next found
                                        if ( aux_hour > 0 || aux_minute > 0 || aux_second > 0 ) {
                                            time.setTime( QTime( 0, 0, 0 ));
                                            data.push_back( std::make_tuple(
                                                time.toMSecsSinceEpoch(),
                                                std::vector<QString>{"","","","","",""} ));
                                            // append the second before the first found
                                            h=aux_hour; m=aux_minute; s=aux_second-1;
                                            if ( s < 0 ) {
                                                s=59; m--;
                                                if ( m < 0 ) {
                                                    m=59; h--;
                                                    if ( h < 0 ) {
                                                        // abort
                                                        h=m=s=0;
                                                    }
                                                }
                                            }
                                            if ( h > 0 || m > 0 || s > 0 ) {
                                                time.setTime( QTime( h, m, s ));
                                                data.push_back( std::make_tuple(
                                                    time.toMSecsSinceEpoch(),
                                                    std::vector<QString>{"","","","","",""} ));
                                            }
                                        }
                                    }
                                    prev_hour = hour;
                                    hour = aux_hour;
                                }
                                prev_minute = minute;
                                minute = aux_minute;
                                prev_second = second;
                                second = aux_second;
                            }
                            tt = query.value(3).toString();
                            ur = query.value(4).toString();
                            qr = query.value(5).toString();
                            mt = query.value(6).toString();
                            pt = query.value(7).toString();
                            rs = query.value(8).toString();
                        }
                        // last one, append the prev
                        h=hour; m=minute; s=second-1;
                        if ( s < 0 ) {
                            s=59; m--;
                            if ( m < 0 ) {
                                m=59; h--;
                                if ( h < 0 ) {
                                    h=m=s=0;
                                }
                            }
                        }
                        if ( prev_hour < h || prev_minute < m || prev_second < s ) {
                            time.setTime( QTime( h, m, s ));
                            data.push_back( std::make_tuple(
                                time.toMSecsSinceEpoch(),
                                std::vector<QString>{"","","","","",""} ));
                        }
                        // append the last count
                        time.setTime( QTime( hour, minute, second ));
                        data.push_back( std::make_tuple(
                            time.toMSecsSinceEpoch(),
                            std::vector<QString>{tt,ur,qr,mt,pt,rs} ));
                        // append 1 second after the last
                        h=hour; m=minute; s=second+1;
                        if ( s > 59 ) {
                            s=0; m++;
                            if ( m > 59 ) {
                                m=0; h++;
                                if ( h > 23 ) {
                                    h=23;m=59;s=59;
                                }
                            }
                        }
                        if ( h > hour || m > minute || s > second ) {
                            time.setTime( QTime( h, m, s ));
                            data.push_back( std::make_tuple(
                                time.toMSecsSinceEpoch(),
                                std::vector<QString>{"","","","","",""} ));
                        }
                        // append the real last fictitious count
                        day ++;
                        if ( day > this->getMonthDays( year, month ) ) {
                            day = 1;
                            month ++;
                            if ( month > 12 ) {
                                month = 1;
                                year ++;
                            }
                        }
                        time.setDate( QDate( year, month , day ) );
                        time.setTime( QTime( 0, 0, 0 ));
                        data.push_back( std::make_tuple(
                            time.toMSecsSinceEpoch(),
                            std::vector<QString>{"","","","","",""} ));
                    }
                } catch (...) {
                    // something failed
                    successful = false;
                    DialogSec::errGeneric( TR::tr(this->MSG_ERR_PROCESSING.c_str()), true );
                }
            }
        }
    }

    if ( ! successful ) {
        data.clear();
    }
    if ( db.isOpen() ) {
        db.close();
    }
    result = std::make_tuple( successful, data );
}



// get, group and count identical items of a specific field in a date
void DbQuery::getItemsCount( std::tuple<bool, std::vector<std::tuple<QString, int>>>& result, const QString& web_server, const QString& year, const QString& month, const QString& day, const QString& log_field )
{
    bool successful = true;
    QHash<QString, int> aux_items;
    std::vector<std::tuple<QString, int>> items;

    QSqlDatabase db;
    if ( QSqlDatabase::contains("qt_sql_default_connection") ) {
        db = QSqlDatabase::database("qt_sql_default_connection");
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE");
    }
    db.setDatabaseName( QString::fromStdString( this->db_path ));

    if ( ! db.open() ) {
        // error opening database
        successful = false;
        QString err_msg = "";
        if ( this->dialog_level == 2 ) {
            err_msg = db.lastError().text();
        }
        DialogSec::errDatabaseFailedOpening( this->db_name, err_msg );

    } else {
        if ( web_server != "apache" && web_server != "nginx" && web_server != "iis" ) {
            // unexpected WebServer
            successful = false;
            DialogSec::errGeneric( QString("%1:\n%2").arg( TR::tr(this->MSG_ERR_UNX_WS.c_str()), web_server ), true );
        }
        if ( successful ) {
            // build the query statement
            QSqlQuery query = QSqlQuery( db );
            QString stmt = QString("SELECT \"%1\" FROM \"%2\" WHERE \"%3\" IS NOT NULL AND \"year\"=%4 AND \"month\"=%5 AND \"day\"=%6;")
                .arg( this->getDbField( log_field ),
                      web_server,
                      this->getDbField( log_field ),
                      year,
                      QString::fromStdString( std::to_string( this->getMonthNumber( month ) )),
                      day );
            // quary the database
            if ( ! query.exec( stmt.replace("'","''") ) ) {
                // error querying database
                successful = false;
                DialogSec::errDatabaseFailedExecuting( this->db_name, query.lastQuery(), query.lastError().text() );

            } else {
                try {
                    // get data
                    QString item;
                    while ( query.next() ) {
                        item = query.value(0).toString();
                        if ( item.size() > 0 ) {
                            if ( aux_items.find( item ) == aux_items.end() ) {
                                // not present yet
                                aux_items.insert( item, 1 );
                            } else {
                                // already in the list, increase the count
                                aux_items[ item ] ++;
                            }
                        }
                    }
                } catch (...) {
                    // something failed
                    successful = false;
                    DialogSec::errGeneric( TR::tr(this->MSG_ERR_PROCESSING.c_str()), true );
                }
            }
        }
        if ( successful ) {
            // sort the list
            std::tuple<QString, int> item;
            // morph tha HashMap into a Vector of Tuples
            foreach ( const QString& v, aux_items.keys() ) {
                item = std::make_tuple( v, aux_items.value( v ) );
                items.push_back( item );
            }
            // sort the vector
            for ( int i=0; i<items.size(); i++ ) {
                for ( int j=0; j<items.size(); j++ ) {
                    if ( std::get<1>(items.at( i )) > std::get<1>(items.at( j )) ) {
                        item = items.at( i );
                        items.at( i ) = items.at( j );
                        items.at( j ) = item;
                    }
                }
            }
        }
        aux_items.clear();
    }

    if ( ! successful ) {
        items.clear();
    }
    if ( db.isOpen() ) {
        db.close();
    }
    result = std::make_tuple( successful, items );
}



// get and count items with a 10 minutes gap for every hour of the day
void DbQuery::getDaytimeCounts( std::tuple<bool, std::unordered_map<int, std::unordered_map<int, int>>>& result, const QString& web_server, const QString& from_year_, const QString& from_month_, const QString& from_day_, const QString& to_year_, const QString& to_month_, const QString& to_day_, const QString& log_field_, const QString& field_filter )
{
    bool successful = true;
    std::unordered_map<int, std::unordered_map<int, int>> data = {
        {0,  {{0,0},{10,0},{20,0},{30,0},{40,0},{50,0}}},  {1,  {{0,0},{10,0},{20,0},{30,0},{40,0},{50,0}}},
        {2,  {{0,0},{10,0},{20,0},{30,0},{40,0},{50,0}}},  {3,  {{0,0},{10,0},{20,0},{30,0},{40,0},{50,0}}},
        {4,  {{0,0},{10,0},{20,0},{30,0},{40,0},{50,0}}},  {5,  {{0,0},{10,0},{20,0},{30,0},{40,0},{50,0}}},
        {6,  {{0,0},{10,0},{20,0},{30,0},{40,0},{50,0}}},  {7,  {{0,0},{10,0},{20,0},{30,0},{40,0},{50,0}}},
        {8,  {{0,0},{10,0},{20,0},{30,0},{40,0},{50,0}}},  {9,  {{0,0},{10,0},{20,0},{30,0},{40,0},{50,0}}},
        {10, {{0,0},{10,0},{20,0},{30,0},{40,0},{50,0}}},  {11, {{0,0},{10,0},{20,0},{30,0},{40,0},{50,0}}},
        {12, {{0,0},{10,0},{20,0},{30,0},{40,0},{50,0}}},  {13, {{0,0},{10,0},{20,0},{30,0},{40,0},{50,0}}},
        {14, {{0,0},{10,0},{20,0},{30,0},{40,0},{50,0}}},  {15, {{0,0},{10,0},{20,0},{30,0},{40,0},{50,0}}},
        {16, {{0,0},{10,0},{20,0},{30,0},{40,0},{50,0}}},  {17, {{0,0},{10,0},{20,0},{30,0},{40,0},{50,0}}},
        {18, {{0,0},{10,0},{20,0},{30,0},{40,0},{50,0}}},  {19, {{0,0},{10,0},{20,0},{30,0},{40,0},{50,0}}},
        {20, {{0,0},{10,0},{20,0},{30,0},{40,0},{50,0}}},  {21, {{0,0},{10,0},{20,0},{30,0},{40,0},{50,0}}},
        {22, {{0,0},{10,0},{20,0},{30,0},{40,0},{50,0}}},  {23, {{0,0},{10,0},{20,0},{30,0},{40,0},{50,0}}},
    };

    QSqlDatabase db;
    if ( QSqlDatabase::contains("qt_sql_default_connection") ) {
        db = QSqlDatabase::database("qt_sql_default_connection");
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE");
    }
    db.setDatabaseName( QString::fromStdString( this->db_path ));

    if ( ! db.open() ) {
        // error opening database
        successful = false;
        QString err_msg = "";
        if ( this->dialog_level == 2 ) {
            err_msg = db.lastError().text();
        }
        DialogSec::errDatabaseFailedOpening( this->db_name, err_msg );

    } else {
        if ( web_server != "apache" && web_server != "nginx" && web_server != "iis" ) {
            // unexpected WebServer
            successful = false;
            DialogSec::errGeneric( QString("%1:\n%2").arg( TR::tr(this->MSG_ERR_UNX_WS.c_str()), web_server ), true );
        }
        int from_year, from_month, from_day,
            to_year, to_month, to_day;
        if ( successful ) {
            // setup period limits
            try {
                from_year  = from_year_.toInt();
                from_month = this->getMonthNumber( from_month_ );
                from_day   = from_day_.toInt();
                to_year  = ( to_year_.size() == 0 )  ? from_year  : to_year_.toInt() ;
                to_month = ( to_month_.size() == 0 ) ? from_month : this->getMonthNumber( to_month_ ) ;
                to_day   = ( to_day_.size() == 0 )   ? from_day   : to_day_.toInt() ;
            } catch (...) {
                // failed to convert to integers
                successful = false;
                DialogSec::errGeneric( TR::tr(this->MSG_ERR_PROCESSING_DATES.c_str()), true );
            }
        }
        if ( successful ) {
            // build the query statement
            QSqlQuery query = QSqlQuery( db );
            QString stmt;
            QString log_field = this->getDbField( log_field_ );

            int n_days   = 0,
                n_months = this->getMonthsCount( from_year, from_month, to_year, to_month );

            int year = from_year,
                month = from_month,
                day, hour, minute;
            QList<int> days_l;

            if ( n_months == 1 ) {
                // 1 month, no need to loop
                stmt = QString("SELECT \"day\", \"hour\", \"minute\" FROM \"%1\" WHERE \"year\"=%2 AND \"month\"=%3 AND \"day\">=%4 AND \"day\"<=%5")
                    .arg( web_server )
                    .arg( year ).arg( month )
                    .arg( from_day ).arg( to_day );

                // apply a filter if present
                if ( field_filter.size() > 0 ) {
                    QString filter = field_filter;
                    if ( filter == "NULL" ) {
                        // only select NULL values
                        stmt += QString(" AND \"%1\" IS NULL")
                            .arg( log_field.replace("'","''") );
                    } else if ( filter == "NOT NULL" ) {
                        // only select non-NULL values
                        stmt += QString(" AND \"%1\" IS NOT NULL")
                            .arg( log_field.replace("'","''") );
                    } else {
                        // filter
                        if ( log_field == "response"
                          || log_field == "time_taken"
                          || log_field == "bytes_sent"
                          || log_field == "bytes_received" ) {
                            // numbers
                            if ( StringOps::isNumeric( field_filter.toStdString() ) ) {
                                // no operator found, set defult to '='
                                filter = QString("=%1").arg( field_filter );
                            }
                            stmt += QString(" AND \"%1\"%2")
                                .arg( log_field.replace("'","''"),
                                      filter.replace("'","''") );

                        } else {
                            stmt += QString(" AND \"%1\" LIKE '%2' || '%'")
                                .arg( log_field.replace("'","''"),
                                      filter.replace("'","''") );
                        }
                    }
                }

                stmt += ";";
                if ( ! query.exec( stmt ) ) {
                    // error querying database
                    successful = false;
                    DialogSec::errDatabaseFailedExecuting( this->db_name, query.lastQuery(), query.lastError().text() );

                } else {
                    try {
                        // clear the list of found days
                        days_l.clear();
                        // get query data
                        while ( query.next() ) {
                            day    = query.value(0).toInt();
                            hour   = query.value(1).toInt();
                            minute = query.value(2).toInt();
                            // increase the count
                            data.at( hour ).at( this->getMinuteGap( minute ) ) ++;
                            // append the day as newly found if not found yet
                            if ( days_l.indexOf( day ) < 0 ) {
                                days_l.push_back( day );
                            }
                        }
                        n_days += days_l.size();
                    } catch (...) {
                        // something failed
                        successful = false;
                        DialogSec::errGeneric( TR::tr(this->MSG_ERR_PROCESSING.c_str()), true );
                    }
                }


            } else {
                for ( int m=1; m<=n_months; m++ ) {
                    stmt = QString("SELECT \"day\", \"hour\", \"minute\" FROM \"%1\" WHERE \"year\"=%2 AND \"month\"=%3")
                        .arg( web_server )
                        .arg( year ).arg( month );
                    if ( m == 1 ) {
                        // first month, only get the day from the beginning day
                        stmt += QString(" AND \"day\">=%1").arg( from_day );
                    } else if ( m == n_months ) {
                        // last month, only get the days until the ending day
                        stmt += QString(" AND \"day\"<=%1").arg( to_day );
                    }

                    // apply a filter if present
                    if ( field_filter.size() > 0 ) {
                        QString filter = field_filter;
                        if ( filter == "NULL" ) {
                            // only select NULL values
                            stmt += QString(" AND \"%1\" IS NULL")
                                .arg( log_field.replace("'","''") );
                        } else if ( filter == "NOT NULL" ) {
                            // only select non-NULL values
                            stmt += QString(" AND \"%1\" IS NOT NULL")
                                .arg( log_field.replace("'","''") );
                        } else {
                            // filter
                            if ( log_field == "warning"
                              || log_field == "response"
                              || log_field == "time_taken"
                              || log_field == "bytes_sent"
                              || log_field == "bytes_received" ) {
                                // numbers
                                if ( StringOps::isNumeric( field_filter.toStdString() ) ) {
                                    // no operator found, set defult to '='
                                    filter = QString("=%1").arg( field_filter );
                                }
                                stmt += QString(" AND \"%1\"%2")
                                    .arg( log_field.replace("'","''"),
                                          filter.replace("'","''") );

                            } else {
                                // only values starting-with
                                stmt += QString(" AND \"%1\" LIKE '%2' || '%'")
                                    .arg( log_field.replace("'","''"),
                                          filter.replace("'","''") );
                            }
                        }
                    }

                    // quary the database
                    stmt += ";";
                    if ( ! query.exec( stmt ) ) {
                        // error querying database
                        successful = false;
                        DialogSec::errDatabaseFailedExecuting( this->db_name, query.lastQuery(), query.lastError().text() );

                    } else {
                        try {
                            // clear the list of found days
                            days_l.clear();
                            // get query data
                            while ( query.next() ) {
                                day    = query.value(0).toInt();
                                hour   = query.value(1).toInt();
                                minute = query.value(2).toInt();
                                // increase the count
                                data.at( hour ).at( this->getMinuteGap( minute ) ) ++;
                                // append the day as newly found if not found yet
                                if ( days_l.indexOf( day ) < 0 ) {
                                    days_l.push_back( day );
                                }
                            }
                            n_days += days_l.size();
                            month ++;
                            if ( month > 12 ) {
                                month = 1;
                                year ++;
                            }
                        } catch (...) {
                            // something failed
                            successful = false;
                            DialogSec::errGeneric( TR::tr(this->MSG_ERR_PROCESSING.c_str()), true );
                            break;
                        }
                        query.finish();
                    }
                }
            }
            if ( successful && n_days > 0 ) {
                // divide the count by the number of days to get the mean value
                for ( const auto& [h,data_] : data ) {
                    for ( const auto& [m,c] : data_ ) {
                        data.at( h ).at( m ) /= n_days;
                    }
                }
            }
        }
    }


    if ( ! successful ) {
        data.clear();
    }
    if ( db.isOpen() ) {
        db.close();
    }
    result = std::make_tuple( successful, data );
}



// get and count how many times a specific item value brought to another
void DbQuery::getRelationalCountsDay(std::tuple<bool, std::vector<std::tuple<long long, int>>> &result, const QString& web_server, const QString& year_, const QString& month_, const QString& day_, const QString& log_field_1_, const QString& field_filter_1, const QString& log_field_2_, const QString& field_filter_2 )
{
    bool successful = true;
    std::vector<std::tuple<long long, int>> data;

    QSqlDatabase db;
    if ( QSqlDatabase::contains("qt_sql_default_connection") ) {
        db = QSqlDatabase::database("qt_sql_default_connection");
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE");
    }
    db.setDatabaseName( QString::fromStdString( this->db_path ));

    if ( ! db.open() ) {
        // error opening database
        successful = false;
        QString err_msg = "";
        if ( this->dialog_level == 2 ) {
            err_msg = db.lastError().text();
        }
        DialogSec::errDatabaseFailedOpening( this->db_name, err_msg );

    } else {
        if ( web_server != "apache" && web_server != "nginx" && web_server != "iis" ) {
            // unexpected WebServer
            successful = false;
            DialogSec::errGeneric( QString("%1:\n%2").arg( TR::tr(this->MSG_ERR_UNX_WS.c_str()), web_server ), true );
        }

        int year, month, day;
        if ( successful ) {
            // setup period limits
            try {
                year  = year_.toInt();
                month = this->getMonthNumber( month_ );
                day   = day_.toInt();
            } catch (...) {
                // failed to convert to integers
                successful = false;
                DialogSec::errGeneric( TR::tr(this->MSG_ERR_PROCESSING_DATES.c_str()), true );
            }
        }
        if ( successful ) {
            QDateTime time;
            time.setDate( QDate( year, month , day ) );
            // build the query statement
            QSqlQuery query = QSqlQuery( db );
            QString stmt;
            QString log_field_1 = this->getDbField( log_field_1_ ),
                    log_field_2 = this->getDbField( log_field_2_ );

            // 1 month, no need to loop
            stmt = QString("SELECT \"hour\", \"minute\" FROM \"%1\" WHERE \"year\"=%2 AND \"month\"=%3 AND \"day\"=%4")
                .arg( web_server )
                .arg( year ).arg( month ).arg( day );

            // apply a filter if present
            if ( field_filter_1.size() > 0 ) {
                QString filter = field_filter_1;
                if ( filter == "NULL" ) {
                    // only select NULL values
                    stmt += QString(" AND \"%1\" IS NULL")
                        .arg( log_field_1.replace("'","''") );
                } else if ( filter == "NOT NULL" ) {
                    // only select non-NULL values
                    stmt += QString(" AND \"%1\" IS NOT NULL")
                        .arg( log_field_1.replace("'","''") );
                } else {
                    // filter
                    if ( log_field_1 == "warning"
                      || log_field_1 == "response"
                      || log_field_1 == "time_taken"
                      || log_field_1 == "bytes_sent"
                      || log_field_1 == "bytes_received" ) {
                        // numbers
                        if ( StringOps::isNumeric( field_filter_1.toStdString() ) ) {
                            // no operator found, set defult to '='
                            filter = QString("=%1").arg( field_filter_1 );
                        }
                        stmt += QString(" AND \"%1\"%2")
                            .arg( log_field_1.replace("'","''"),
                                  filter.replace("'","''") );

                    } else {
                        // only values starting-with
                        stmt += QString(" AND \"%1\" LIKE '%2' || '%'")
                            .arg( log_field_1.replace("'","''"),
                                  filter.replace("'","''") );
                    }
                }
            }
            // apply a filter if present
            if ( field_filter_2.size() > 0 ) {
                QString filter = field_filter_2;
                if ( filter == "NULL" ) {
                    // only select NULL values
                    stmt += QString(" AND \"%1\" IS NULL")
                        .arg( log_field_2.replace("'","''") );
                } else if ( filter == "NOT NULL" ) {
                    // only select non-NULL values
                    stmt += QString(" AND \"%1\" IS NOT NULL")
                        .arg( log_field_2.replace("'","''") );
                } else {
                    // filter
                    if ( log_field_2 == "warning"
                      || log_field_2 == "response"
                      || log_field_2 == "time_taken"
                      || log_field_2 == "bytes_sent"
                      || log_field_2 == "bytes_received" ) {
                        // numbers
                        if ( StringOps::isNumeric( field_filter_2.toStdString() ) ) {
                            // no operator found, set defult to '='
                            filter = QString("=%1").arg( field_filter_2 );
                        }
                        stmt += QString(" AND \"%1\"%2")
                            .arg( log_field_2.replace("'","''"),
                                  filter.replace("'","''") );

                    } else {
                        // only values starting-with
                        stmt += QString(" AND \"%1\" LIKE '%2' || '%'")
                            .arg( log_field_2.replace("'","''"),
                                  QString(field_filter_2).replace("'","''") );
                    }
                }
            }

            stmt += QString(" ORDER BY \"hour\",\"minute\" ASC;");
            if ( ! query.exec( stmt ) ) {
                // error querying database
                successful = false;
                DialogSec::errDatabaseFailedExecuting( this->db_name, query.lastQuery(), query.lastError().text() );

            } else {
                try {
                    // get query data
                    int hour, aux_hour, minute, aux_minute, count;
                    int gap = 20;
                    hour = -1;
                    minute = count = 0;
                    if ( query.size() == 0 ) {
                        // no result found, append missing days with 0 value
                        for ( int h=0; h<24; h++ ) {
                            for ( int m=0; m<60; m+=gap ) {
                                time.setTime( QTime( h, m ) );
                                data.push_back( std::make_tuple( time.toMSecsSinceEpoch(), 0 ) );
                            }
                        }

                    } else {
                        while ( query.next() ) {
                            aux_hour   = query.value(0).toInt();
                            aux_minute = this->getMinuteGap( query.value(1).toInt(), gap );
                            if ( aux_hour == hour && aux_minute == minute ) {
                                count ++;
                            } else {
                                if ( aux_hour == hour ) {
                                    // same hour new minute gap, append the last count
                                    time.setTime( QTime( hour, minute ) );
                                    data.push_back( std::make_tuple( time.toMSecsSinceEpoch(), count ) );
                                    // and any missing gap
                                    for ( int m=minute+gap; m<aux_minute; m+=gap ) {
                                        time.setTime( QTime( hour, m ) );
                                        data.push_back( std::make_tuple( time.toMSecsSinceEpoch(), 0 ) );
                                    }
                                } else {
                                    // minute is always different when the hour is different
                                    if ( hour >= 0 ) {
                                        // apend the last minute-gap count if not in the first round of the loop
                                        time.setTime( QTime( hour, minute ) );
                                        data.push_back( std::make_tuple( time.toMSecsSinceEpoch(), count ) );
                                        // append any missing gap in the current hour
                                        for ( int m=minute+gap; m<60; m+=gap ) {
                                            time.setTime( QTime( hour, m ) );
                                            data.push_back( std::make_tuple( time.toMSecsSinceEpoch(), 0 ) );
                                        }
                                        hour ++;
                                    } else {
                                        // prepare to add missing gaps from 00:00 (+gap will be added to the minute)
                                        hour = 0;
                                    }
                                    // append any missing gap in every hour between the current and the next found (aux)
                                    for ( int h=hour; h<aux_hour; h++ ) {
                                        for ( int m=0; m<60; m+=gap ) {
                                            time.setTime( QTime( h, m ) );
                                            data.push_back( std::make_tuple( time.toMSecsSinceEpoch(), 0 ) );
                                        }
                                    }
                                    // append any missing gap in the netx found hour
                                    for ( int m=0; m<aux_minute; m+=gap ) {
                                        time.setTime( QTime( aux_hour, m ) );
                                        data.push_back( std::make_tuple( time.toMSecsSinceEpoch(), 0 ) );
                                    }
                                    hour = aux_hour;
                                }
                                minute = aux_minute;
                                count = 1;
                            }
                        }
                        // append the last count
                        time.setTime( QTime( hour, minute ) );
                        data.push_back( std::make_tuple( time.toMSecsSinceEpoch(), count ) );
                        // yet again, append any missing gap
                        for ( int m=minute+gap; m<60; m+=gap ) {
                            time.setTime( QTime( hour, m ) );
                            data.push_back( std::make_tuple( time.toMSecsSinceEpoch(), 0 ) );
                        }
                        for ( int h=hour+1; h<24; h++ ) {
                            for ( int m=0; m<60; m+=gap ) {
                                time.setTime( QTime( h, m ) );
                                data.push_back( std::make_tuple( time.toMSecsSinceEpoch(), 0 ) );
                            }
                        }
                        // append the real last fictitious count
                        day ++;
                        if ( day > this->getMonthDays( year, month ) ) {
                            day = 1;
                            month ++;
                            if ( month > 12 ) {
                                month = 1;
                                year ++;
                            }
                        }
                        time.setDate( QDate( year, month , day ) );
                        time.setTime( QTime( 0, 0 ) );
                        //time.setTime( QTime( 23, 59, 59, 999 ) );
                        data.push_back( std::make_tuple( time.toMSecsSinceEpoch(), 0 ) );
                    }
                } catch (...) {
                    // something failed
                    successful = false;
                    DialogSec::errGeneric( TR::tr(this->MSG_ERR_PROCESSING.c_str()), true );
                }
            }
        }
    }

    if ( ! successful ) {
        data.clear();
    }
    if ( db.isOpen() ) {
        db.close();
    }
    result = std::make_tuple( successful, data );
}



void DbQuery::getRelationalCountsPeriod(std::tuple<bool, std::vector<std::tuple<long long, int>>> &result, const QString& web_server, const QString& from_year_, const QString& from_month_, const QString& from_day_, const QString& to_year_, const QString& to_month_, const QString& to_day_, const QString& log_field_1_, const QString& field_filter_1, const QString& log_field_2_, const QString& field_filter_2 )
{
    bool successful = true;
    std::vector<std::tuple<long long, int>> data;

    QSqlDatabase db;
    if ( QSqlDatabase::contains("qt_sql_default_connection") ) {
        db = QSqlDatabase::database("qt_sql_default_connection");
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE");
    }
    db.setDatabaseName( QString::fromStdString( this->db_path ));

    if ( ! db.open() ) {
        // error opening database
        successful = false;
        QString err_msg = "";
        if ( this->dialog_level == 2 ) {
            err_msg = db.lastError().text();
        }
        DialogSec::errDatabaseFailedOpening( this->db_name, err_msg );

    } else {
        if ( web_server != "apache" && web_server != "nginx" && web_server != "iis" ) {
            // unexpected WebServer
            successful = false;
            DialogSec::errGeneric( QString("%1:\n%2").arg( TR::tr(this->MSG_ERR_UNX_WS.c_str()), web_server ), true );
        }
        int from_year, from_month, from_day,
            to_year,   to_month,   to_day;
        if ( successful ) {
            // setup period limits
            try {
                from_year  = from_year_.toInt();
                from_month = this->getMonthNumber( from_month_ );
                from_day   = from_day_.toInt();
                to_year  = ( to_year_.size() == 0 )  ? from_year  : to_year_.toInt() ;
                to_month = ( to_month_.size() == 0 ) ? from_month : this->getMonthNumber( to_month_ ) ;
                to_day   = ( to_day_.size() == 0 )   ? from_day   : to_day_.toInt() ;
            } catch (...) {
                // failed to convert to integers
                successful = false;
                DialogSec::errGeneric( TR::tr(this->MSG_ERR_PROCESSING_DATES.c_str()), true );
            }
        }
        if ( successful ) {
            // build the query statement
            QSqlQuery query = QSqlQuery( db );
            QString stmt;
            QString log_field_1 = this->getDbField( log_field_1_ ),
                    log_field_2 = this->getDbField( log_field_2_ );

            int n_months = this->getMonthsCount( from_year, from_month, to_year, to_month );

            QDateTime time;
            int year  = from_year,
                month = from_month,
                day, aux_day, count;

            if ( n_months == 1 ) {
                // 1 month, no need to loop
                stmt = QString("SELECT \"day\" FROM \"%1\" WHERE \"year\"=%2 AND \"month\"=%3 AND \"day\">=%4 AND \"day\"<=%5")
                    .arg( web_server )
                    .arg( year ).arg( month )
                    .arg( from_day ).arg( to_day );

                // only select non-NULL values
                stmt += QString(" AND \"%1\" IS NOT NULL")
                    .arg( log_field_1.replace("'","''") );
                // apply a filter if present
                if ( field_filter_1.size() > 0 ) {
                    QString filter = field_filter_1;
                    if ( log_field_1 == "response"
                      || log_field_1 == "time_taken"
                      || log_field_1 == "bytes_sent"
                      || log_field_1 == "bytes_received" ) {
                    // numbers
                    if ( StringOps::isNumeric( field_filter_1.toStdString() ) ) {
                        // no operator found, set defult to '='
                        filter = QString("=%1").arg( field_filter_1 );
                    }
                    stmt += QString(" AND \"%1\"%2")
                        .arg( log_field_1.replace("'","''"),
                              filter.replace("'","''") );

                    } else {
                        stmt += QString(" AND \"%1\" LIKE '%2' || '%'")
                            .arg( log_field_1.replace("'","''"),
                                  filter.replace("'","''") );
                    }
                }
                // only select non-NULL values
                stmt += QString(" AND \"%1\" IS NOT NULL")
                    .arg( log_field_2.replace("'","''") );
                // apply a filter if present
                if ( field_filter_2.size() > 0 ) {
                    QString filter = field_filter_2;
                    if ( log_field_2 == "response"
                      || log_field_2 == "time_taken"
                      || log_field_2 == "bytes_sent"
                      || log_field_2 == "bytes_received" ) {
                    // numbers
                    if ( StringOps::isNumeric( field_filter_2.toStdString() ) ) {
                        // no operator found, set defult to '='
                        filter = QString("=%1").arg( field_filter_2 );
                    }
                    stmt += QString(" AND \"%1\"%2")
                        .arg( log_field_2.replace("'","''"),
                              filter.replace("'","''") );

                    } else {
                        stmt += QString(" AND \"%1\" LIKE '%2' || '%'")
                            .arg( log_field_2.replace("'","''"),
                                  QString(field_filter_2).replace("'","''") );
                    }
                }

                stmt += QString(" ORDER BY \"day\" ASC;");
                if ( ! query.exec( stmt ) ) {
                    // error querying database
                    successful = false;
                    DialogSec::errDatabaseFailedExecuting( this->db_name, query.lastQuery(), query.lastError().text() );

                } else {
                    try {
                        // get query data
                        day = count = 0;
                        if ( query.size() == 0 ) {
                            // no days found, append missing days with 0 value
                            for ( int d=from_day; d<=to_day; d++ ) {
                                time.setDate( QDate( year, month , d ) );
                                data.push_back( std::make_tuple( time.toMSecsSinceEpoch(), 0 ) );
                            }

                        } else {
                            while ( query.next() ) {
                                aux_day = query.value(0).toInt();
                                if ( aux_day == day ) {
                                    count ++;
                                } else {
                                    if ( day > 0 ) {
                                        // any loop-round except the first
                                        time.setDate( QDate( year, month , day ) );
                                        data.push_back( std::make_tuple( time.toMSecsSinceEpoch(), count ) );
                                        for ( int d=day+1; d<aux_day; d++ ) {
                                            // append any missing day with a zero value
                                            time.setDate( QDate( year, month , d ) );
                                            data.push_back( std::make_tuple( time.toMSecsSinceEpoch(), 0 ) );
                                        }
                                    } else {
                                        // append any missing day from 1 day before the first until the next found
                                        int d = from_day - 1,
                                            m = month,
                                            y = year;
                                        if ( d < 1 ) {
                                            m --;
                                            if ( m < 1 ) {
                                                m = 12;
                                                y --;
                                            }
                                            d = this->getMonthDays( y, m );
                                        }
                                        for ( ; d!=aux_day; d++ ) {
                                            if ( d > this->getMonthDays( y, m ) ) {
                                                d = 1;
                                                m ++;
                                                if ( m > 12 ) {
                                                    m = 1;
                                                    y ++;
                                                }
                                            }
                                            time.setDate( QDate( y, m , d ) );
                                            data.push_back( std::make_tuple( time.toMSecsSinceEpoch(), 0 ) );
                                        }
                                    }
                                    day = aux_day;
                                    count = 1;
                                }
                            }
                            // append the last count
                            time.setDate( QDate( year, month , day ) );
                            data.push_back( std::make_tuple( time.toMSecsSinceEpoch(), count ) );
                            // append any missing day from the last found until 1 day fater the last one
                            day++;
                            if ( day > this->getMonthDays( year, month ) ) {
                                month ++;
                                if ( month > 12 ) {
                                    month = 1;
                                    year ++;
                                }
                                day = this->getMonthDays( year, month );
                            }
                            to_day += 2;
                            if ( to_day > this->getMonthDays( year, month ) ) {
                                int m = month + 1,
                                    y = year;
                                if ( m > 12 ) {
                                    m = 1;
                                    y ++;
                                }
                                to_day = this->getMonthDays( y, m );
                            }
                            for ( ; day!=to_day; day++ ) {
                                if ( day > this->getMonthDays( year, month ) ) {
                                    day = 1;
                                    month ++;
                                    if ( month > 12 ) {
                                        month = 1;
                                        year ++;
                                    }
                                }
                                time.setDate( QDate( year, month , day ) );
                                data.push_back( std::make_tuple( time.toMSecsSinceEpoch(), 0 ) );
                            }
                        }
                    } catch (...) {
                        // something failed
                        successful = false;
                        DialogSec::errGeneric( TR::tr(this->MSG_ERR_PROCESSING.c_str()), true );
                    }
                }

            } else {
                for ( int m=1; m<=n_months; m++ ) {
                    stmt = QString("SELECT \"day\" FROM \"%1\" WHERE \"year\"=%2 AND \"month\"=%3")
                        .arg( web_server )
                        .arg( year ).arg( month );

                    if ( m == 1 ) {
                        // first month, only get the day from the beginning day
                        stmt += QString(" AND \"day\">=%1").arg( from_day );
                    } else if ( m == n_months ) {
                        // last month, only get the days until the ending day
                        stmt += QString(" AND \"day\"<=%1").arg( to_day );
                    }

                    // only select non-NULL values
                    stmt += QString(" AND \"%1\" IS NOT NULL")
                        .arg( log_field_1.replace("'","''") );
                    // apply a filter if present
                    if ( field_filter_1.size() > 0 ) {
                        QString filter = field_filter_1;
                        if ( log_field_1 == "response"
                          || log_field_1 == "time_taken"
                          || log_field_1 == "bytes_sent"
                          || log_field_1 == "bytes_received" ) {
                            // numbers
                            if ( StringOps::isNumeric( field_filter_1.toStdString() ) ) {
                                // no operator found, set defult to '='
                                filter = QString("=%1").arg( field_filter_1 );
                            }
                            stmt += QString(" AND \"%1\"%2")
                                .arg( log_field_1.replace("'","''"),
                                      filter.replace("'","''") );

                        } else {
                            stmt += QString(" AND \"%1\" LIKE '%2' || '%'")
                                .arg( log_field_1.replace("'","''"),
                                      filter.replace("'","''") );
                        }
                    }
                    // only select non-NULL values
                    stmt += QString(" AND \"%1\" IS NOT NULL")
                        .arg( log_field_2.replace("'","''") );
                    // apply a filter if present
                    if ( field_filter_2.size() > 0 ) {
                        QString filter = field_filter_2;
                        if ( log_field_2 == "response"
                          || log_field_2 == "time_taken"
                          || log_field_2 == "bytes_sent"
                          || log_field_2 == "bytes_received" ) {
                            // numbers
                            if ( StringOps::isNumeric( field_filter_2.toStdString() ) ) {
                                // no operator found, set defult to '='
                                filter = QString("=%1").arg( field_filter_2 );
                            }
                            stmt += QString(" AND \"%1\"%2")
                                .arg( log_field_2.replace("'","''"),
                                      filter.replace("'","''") );

                        } else {
                            stmt += QString(" AND \"%1\" LIKE '%2' || '%'")
                                .arg( log_field_2.replace("'","''"),
                                      QString(field_filter_2).replace("'","''") );
                        }
                    }

                    // quary the database
                    stmt += " ORDER BY \"day\" ASC;";
                    if ( ! query.exec( stmt ) ) {
                        // error querying database
                        successful = false;
                        DialogSec::errDatabaseFailedExecuting( this->db_name, query.lastQuery(), query.lastError().text() );

                    } else {
                        try {
                            // get query data
                            day = count = 0;
                            if ( query.size() == 0 ) {
                                // no days found, append missing days with 0 value
                                int f_d = 1,
                                    t_d = this->getMonthDays( year, month );
                                if ( m == 1 ) {
                                    // first month, only get the day from the beginning day
                                    f_d = from_day;
                                } else if ( m == n_months ) {
                                    // last month, only get the days until the ending day
                                    t_d = to_day;
                                }
                                for ( ; f_d<=t_d; f_d++ ) {
                                    time.setDate( QDate( year, month , f_d ) );
                                    data.push_back( std::make_tuple( time.toMSecsSinceEpoch(), 0 ) );
                                }

                            } else {
                                while ( query.next() ) {
                                    aux_day = query.value(0).toInt();
                                    if ( aux_day == day ) {
                                        count ++;
                                    } else {
                                        if ( day > 0 ) {
                                            // any loop-round except the first
                                            time.setDate( QDate( year, month , day ) );
                                            data.push_back( std::make_tuple( time.toMSecsSinceEpoch(), count ) );
                                            for ( int d=day+1; d<aux_day; d++ ) {
                                                // append any missing day with a zero value
                                                time.setDate( QDate( year, month , d ) );
                                                data.push_back( std::make_tuple( time.toMSecsSinceEpoch(), 0 ) );
                                            }
                                        } else {
                                            // append any missing day until the next found day
                                            day = 1;
                                            for ( int d=day; d<aux_day; d++ ) {
                                                time.setDate( QDate( year, month , d ) );
                                                data.push_back( std::make_tuple( time.toMSecsSinceEpoch(), 0 ) );
                                            }
                                        }
                                        day = aux_day;
                                        count = 1;
                                    }
                                }
                                // append the last count
                                if ( day > 0 ) {
                                    time.setDate( QDate( year, month , day ) );
                                    data.push_back( std::make_tuple( time.toMSecsSinceEpoch(), count ) );
                                }
                                // append any missing day to the end of the month with a zero value
                                for ( int d=day+1; d<=this->getMonthDays(year,month); d++ ) {
                                    time.setDate( QDate( year, month , d ) );
                                    data.push_back( std::make_tuple( time.toMSecsSinceEpoch(), 0 ) );
                                }
                            }
                            // increase the month
                            month ++;
                            if ( month > 12 ) {
                                month = 1;
                                year ++;
                            }
                        } catch (...) {
                            // something failed
                            successful = false;
                            DialogSec::errGeneric( TR::tr(this->MSG_ERR_PROCESSING.c_str()), true );
                            break;
                        }
                    }
                    query.finish();
                }
                // append the first day of 1 month after the last one as the last one day
                day = 1;
                month = to_month +1;
                if ( month > 12 ) {
                    month = 1;
                    year ++;
                }
                time.setDate( QDate( year, month , day ) );
                data.push_back( std::make_tuple( time.toMSecsSinceEpoch(), 0 ) );
            }
        }
    }

    if ( ! successful ) {
        data.clear();
    }
    if ( db.isOpen() ) {
        db.close();
    }
    result = std::make_tuple( successful, data );
}



const bool DbQuery::getGlobalCounts( const QString& web_server, const std::unordered_map<int, std::unordered_map<int, std::vector<int>>>& dates, std::vector<std::unordered_map<QString, int>>& recurs, std::tuple<QString, int>& traf_date, std::unordered_map<int, double>& traf_day, std::unordered_map<int, double>& traf_hour, std::vector<long long>& perf_time, std::vector<long long>& perf_sent, std::vector<long long>& perf_receiv, long& req_count )
{
    bool successful = true;

    QSqlDatabase db;
    if ( QSqlDatabase::contains("qt_sql_default_connection") ) {
        db = QSqlDatabase::database("qt_sql_default_connection");
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE");
    }
    db.setDatabaseName( QString::fromStdString( this->db_path ));

    if ( ! db.open() ) {
        // error opening database
        successful = false;
        QString err_msg = "";
        if ( this->dialog_level == 2 ) {
            err_msg = db.lastError().text();
        }
        DialogSec::errDatabaseFailedOpening( this->db_name, err_msg );

    } else {
        if ( web_server != "apache" && web_server != "nginx" && web_server != "iis" ) {
            // unexpected WebServer
            successful = false;
            DialogSec::errGeneric( QString("%1:\n%2").arg( TR::tr(this->MSG_ERR_UNX_WS.c_str()), web_server ), true );
        }
    }

    if ( successful ) {
        QSqlQuery query = QSqlQuery( db );
        int d, h, tt, bs, br,
            day, hour, week_day,
            day_count, hour_count,
            max_date_count=0,
            max_tt=0, tot_tt=0, num_tt=0,
            max_bs=0, tot_bs=0, num_bs=0,
            max_br=0, tot_br=0, num_br=0,
            n_days=0;
        QString protocol, method, uri, user_agent, max_date_str="";
        std::unordered_map<int, int> num_day_count = {
            {1,0}, {2,0}, {3,0}, {4,0}, {5,0}, {6,0}, {7,0} };
        // get years
        for ( const auto& [year, dates_] : dates ) {
            // get months of the year
            for ( const auto& [month, dates__] : dates_ ) {

                hour=-1; hour_count=0;
                day=-1; day_count=0;

                if ( ! query.exec( QString("SELECT \"day\",\"hour\",\"protocol\",\"method\",\"uri\",\"user_agent\",\"time_taken\",\"bytes_sent\",\"bytes_received\" FROM \"%1\" WHERE \"year\"=%2 AND \"month\"=%3 ORDER BY \"day\",\"hour\" ASC;")
                                          .arg( web_server ).arg( year ).arg( month ).replace("'","''") ) ) {
                    // error querying database
                    successful = false;
                    DialogSec::errDatabaseFailedExecuting( this->db_name, query.lastQuery(), query.lastError().text() );
                    break;

                } else {
                    while ( query.next() ) {
                        try {
                            // day
                            if ( query.value(0).isNull() ) {
                                d = -1;
                            } else {
                                d = query.value(0).toInt();
                            }
                            // hour
                            if ( query.value(1).isNull() ) {
                                h = -1;
                            } else {
                                h = query.value(1).toInt();
                            }
                            // protocol
                            if ( query.value(2).isNull() ) {
                                protocol = "";
                            } else {
                                protocol = query.value(2).toString();
                            }
                            // method
                            if ( query.value(3).isNull() ) {
                                method = "";
                            } else {
                                method = query.value(3).toString();
                            }
                            // uri
                            if ( query.value(4).isNull() ) {
                                uri = "";
                            } else {
                                uri = query.value(4).toString();
                            }
                            // user agent
                            if ( query.value(5).isNull() ) {
                                user_agent = "";
                            } else {
                                user_agent = query.value(5).toString();
                            }
                            // time taken
                            if ( query.value(6).isNull() ) {
                                tt = -1;
                            } else {
                                tt = query.value(6).toInt();
                            }
                            // bytes sent
                            if ( query.value(7).isNull() ) {
                                bs = -1;
                            } else {
                                bs = query.value(7).toInt();
                            }
                            // bytes received
                            if ( query.value(8).isNull() ) {
                                br = -1;
                            } else {
                                br = query.value(8).toInt();
                            }
                        } catch (...) {
                            // failed to convert to integer
                            successful = false;
                            QString err_msg = "";
                            if ( this->dialog_level == 2 ) {
                                err_msg = TR::tr(this->MSG_ERR_PROCESSING.c_str());
                            }
                            DialogSec::errGeneric( err_msg );
                            break;
                        }
                        if ( successful ) {

                            // process the day count
                            if ( d > 0 ) {
                                if ( day == -1 ) {
                                    day = d;
                                }
                                if ( d == day ) {
                                    day_count ++;
                                } else {
                                    n_days ++;
                                    // sum the day count to the total count
                                    req_count += day_count;
                                    // sum the day count to the relative day of the week count
                                    week_day = QDate(year,month,day).dayOfWeek();
                                    traf_day.at( week_day ) += day_count;
                                    num_day_count.at( week_day ) ++;
                                    // check the max date count
                                    const QString m_str = (month<10) ? QString("0%1").arg(month) : QString("%1").arg(month) ;
                                    const QString d_str = (day<10) ? QString("0%1").arg(day) : QString("%1").arg(day) ;
                                    if ( day_count > max_date_count ) {
                                        max_date_count = day_count;
                                        max_date_str = QString("%1-%2-%3").arg( year ).arg( m_str, d_str );
                                    }
                                    day_count = 1;
                                    day = d;
                                }
                            }

                            // process the hour count
                            if ( h >= 0 ) {
                                if ( hour == -1 ) {
                                    hour = h;
                                }
                                if ( h == hour ) {
                                    hour_count ++;
                                } else {
                                    traf_hour.at( hour ) += hour_count;
                                    hour_count = 1;
                                    hour = h;
                                }
                            }

                            // sum the time taken
                            if ( tt >= 0 ) {
                                if ( tt > max_tt ) {
                                    max_tt = tt;
                                }
                                tot_tt += tt;
                                num_tt ++;
                            }

                            // sum the bytes sent
                            if ( bs >= 0 ) {
                                if ( bs > max_bs ) {
                                    max_bs = bs;
                                }
                                tot_bs += bs;
                                num_bs ++;
                            }

                            // sum the bytes received
                            if ( br >= 0 ) {
                                if ( br > max_br ) {
                                    max_br = br;
                                }
                                tot_br += br;
                                num_br ++;
                            }

                            // process the protocol
                            if ( protocol.size() > 0 ) {
                                if ( recurs.at(0).find( protocol ) != recurs.at(0).end() ) {
                                    // sum
                                    recurs.at(0).at( protocol ) ++;
                                } else {
                                    // new
                                    recurs.at(0).emplace( protocol, 1 );
                                }
                            }

                            // process the method
                            if ( method.size() > 0 ) {
                                if ( recurs.at(1).find( method ) != recurs.at(1).end() ) {
                                    // sum
                                    recurs.at(1).at( method ) ++;
                                } else {
                                    // new
                                    recurs.at(1).emplace( method, 1 );
                                }
                            }

                            // process the uri
                            if ( uri.size() > 0 ) {
                                if ( recurs.at(2).find( uri ) != recurs.at(2).end() ) {
                                    // sum
                                    recurs.at(2).at( uri ) ++;
                                } else {
                                    // new
                                    recurs.at(2).emplace( uri, 1 );
                                }
                            }

                            // process the user-agent
                            if ( user_agent.size() > 0 ) {
                                if ( recurs.at(3).find( user_agent ) != recurs.at(3).end() ) {
                                    // sum
                                    recurs.at(3).at( user_agent ) ++;
                                } else {
                                    // new
                                    recurs.at(3).emplace( user_agent, 1 );
                                }
                            }
                        }
                    }
                    // complete the remaining stats
                    if ( successful ) {
                        // append the last hour
                        if ( hour >= 0 ) {
                            traf_hour.at( hour ) += hour_count;
                        }

                        // sum the day count to the total count
                        req_count += day_count;

                        // sum the day count to the relative day of the week count
                        week_day = QDate(year,month,day).dayOfWeek();
                        traf_day.at( week_day ) += day_count;
                        num_day_count.at( week_day ) ++;

                        // check the max date count
                        const QString m_str = (month<10) ? QString("0%1").arg(month) : QString("%1").arg(month) ;
                        const QString d_str = (day<10) ? QString("0%1").arg(day) : QString("%1").arg(day) ;
                        if ( day_count > max_date_count ) {
                            max_date_count = day_count;
                            max_date_str = QString("%1-%2-%3").arg( year ).arg( m_str, d_str );
                        }
                    }
                }
                query.finish();
                if ( ! successful ) { break; }
            }
            if ( ! successful ) { break; }
        }

        // final process for some of the values
        if ( successful ) {

            // process the hours of the day
            for ( int i=0; i<24; i++ ) {
                if ( n_days > 0 ) {
                    traf_hour.at( i ) /= n_days;
                }
            }

            // process the day of the week
            for ( int i=1; i<8; i++ ) {
                const int &x = num_day_count.at( i );
                if ( x > 0 ) {
                    traf_day.at( i ) /= x;
                }
            }

            // make the max-date tuple
            traf_date = std::make_tuple( max_date_str, max_date_count );

            // time-taken perfs
            perf_time = { max_tt, tot_tt, num_tt };
            // bytes-sent perfs
            perf_sent = { max_bs, tot_bs, num_bs };
            // bytes-received perfs
            perf_receiv = { max_br, tot_br, num_br };
        }
    }

    return successful;
}

