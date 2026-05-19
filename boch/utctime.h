#pragma once
struct utctm {
	//88
	Bit16s tm_sec;  // seconds after the minute 0-59
	Bit16s tm_min;  // minutes after the hour   0-59
	Bit16s tm_hour; // hours since midnight     0-23
	Bit16s tm_mday; // day of the month         1-31
	Bit16s tm_mon;  // months since January     0-11
	Bit16s tm_year; // years since 1900
	Bit16s tm_wday; // days since Sunday        0-6
	Bit16s tm_yday; // days since January 1     0-365
};
struct utctm* utctime_ext(const Bit64s* a, struct utctm* trgt)
{
	//108
	//Days elapsed between the start of the selected month and the start of the year
	static const Bit32s monthlydays[2][13] = {
	   {0,31,59,90,120,151,181,212,243,273,304,334,365},
	   {0,31,60,91,121,152,182,213,244,274,305,335,366}
	};
	Bit8u isleap = 0;                                                       //Leap year flag
	struct utctm bdt;                                                     //Structure to temporary store the output
	Bit64s etmp = *a;                                                       //Temporary variable, epoch since 1970
	Bit64s tsec;                                                          //Temporary variable, seconds
	Bit64s eyear = 2001;                                                    //Temporary variable, year

	tsec = etmp % (24 * 60 * 60);                                                 //Get time of day
	etmp /= (24 * 60 * 60);                                                     //Get days number
	etmp -= 11323;                                                          //Use 2001-01-01 as the base of the days number, it being the nearest first non-leap year of a 400yrs cycle
	if (tsec < 0) { etmp--; tsec += (24 * 60 * 60); };                                 //Get a positive time_of_day number and a properly rounded days number

	bdt.tm_sec = tsec % 60;                                                   //Set the seconds value
	tsec /= 60;
	bdt.tm_min = tsec % 60;                                                   //Set the minutes value
	tsec /= 60;
	bdt.tm_hour = (Bit16s)tsec;                                           //Set the hour value

	bdt.tm_wday = (etmp - 6) % 7;
	if (bdt.tm_wday < 0) bdt.tm_wday += 7;                                     //Set the day of the week value

	if (etmp < 0) { eyear += 400 * (etmp / 146097 - 1); etmp %= 146097; etmp += 146097; };    //Years before 2001 accounted for

	eyear += 400 * (etmp / 146097);                                             //Add the number of 400yr cycles
	etmp %= 146097;
	eyear += 100 * (etmp / 36524);                                              //Add the number of 100yr cycles
	etmp %= 36524;
	eyear += 4 * (etmp / 1461);                                                 //Add the number of   4yr cycles
	etmp %= 1461;
	while ((eyear % 4) && (etmp >= 365)) { eyear++; etmp -= 365; }                     //Add the number of remaining years;

	isleap |= ((eyear % 400) ? 0 : 2);
	isleap |= ((eyear % 4) ? 0 : 1);
	isleap &= ((eyear % 100) ? ~0 : ~1);
	isleap = (isleap ? 1 : 0);                                                  //Find out if the year is leap

	eyear -= 1900;
	bdt.tm_year = (Bit16s)eyear;                                          //Set the year value

	bdt.tm_yday = (Bit16s)etmp;                                           //Set the day of the year value
	bdt.tm_mon = 0;
	while (etmp >= monthlydays[isleap][bdt.tm_mon + 1]) bdt.tm_mon++;          //Set the month value
	etmp -= monthlydays[isleap][bdt.tm_mon];
	bdt.tm_mday = (Bit16s)(etmp + 1);                                     //Set the day of the month value

	if (eyear != bdt.tm_year) return NULL;                                 //If the calculated year is too high fail

	trgt->tm_sec = bdt.tm_sec;                                            //Else write back in the structure proper values
	trgt->tm_min = bdt.tm_min;                                            //And return its address
	trgt->tm_hour = bdt.tm_hour;
	trgt->tm_wday = bdt.tm_wday;
	trgt->tm_yday = bdt.tm_yday;
	trgt->tm_mday = bdt.tm_mday;
	trgt->tm_mon = bdt.tm_mon;
	trgt->tm_year = bdt.tm_year;
	return trgt;
}

Bit64s timeutc(struct utctm* bdt)
{
	//Days elapsed between the start of the selected month and the start of the year
	static const Bit32s monthlydays[2][13] = {
	   {0,31,59,90,120,151,181,212,243,273,304,334,365},
	   {0,31,60,91,121,152,182,213,244,274,305,335,366}
	};
	Bit8u   isleap = 3;                                                     //Leap year flag
	Bit32s  tmon;                                                         //Temporary month value
	Bit64s  epoch = 0;                                                      //Value to return
	Bit64s  etmp;                                                         //Temporary variable

	etmp = bdt->tm_year;
	tmon = bdt->tm_mon;

	etmp += tmon / 12;
	tmon %= 12;
	if (tmon < 0) { etmp--; tmon += 12; };                                         //Broken month value accounted for

	etmp -= 101;                                                            //Years passed since 2001
	if (etmp < 0) { epoch += (146097 * (etmp / 400 - 1)); etmp %= 400; etmp += 400; };    //Years before 2001 accounted for

	epoch += (etmp / 400) * 146097;                                             //Add in epoch the number of days corresponding to completed 400yr cycles
	etmp %= 400;
	isleap &= ((etmp == 399) ? ~0 : ~2);                                          //Clear bit1 if the year can not be divided by 400
	epoch += (etmp / 100) * 36524;                                              //Add in epoch the number of days corresponding to completed 100yr cycles
	etmp %= 100;
	isleap &= ((etmp == 99) ? ~1 : ~0);                                           //Clear bit0 if the year can be divided by 100
	epoch += (etmp / 4) * 1461;                                                 //Add in epoch the number of days corresponding to completed   4yr cycles
	etmp %= 4;
	isleap &= ((etmp == 3) ? ~0 : ~1);                                            //Clear bit0 if the year can not divided by 4
	isleap = (isleap ? 1 : 0);                                                  //Shrink the flag to a single bit
	epoch += etmp * 365;                                                      //Add in epoch the number of days corresponding to completed years

	//Now we have in epoch the number (positive or negative) of days between the start of the current year and the start of 2001 and isleap set if the year is leap

	epoch += monthlydays[isleap][tmon];
	epoch += bdt->tm_mday - 1;                                                //Now we have in epoch the number of entire days between the current date and 2001-01-01 00:00
	epoch *= 24;
	epoch += bdt->tm_hour;                                                  //Now we have in epoch the number of hours
	epoch *= 60;
	epoch += bdt->tm_min;                                                   //Now we have in epoch the number of minutes
	epoch *= 60;
	epoch += bdt->tm_sec;                                                   //Now we have a positive or negative number of seconds between the input time and 2001-01-01 00:00:00
	epoch += 978307200;                                                     //Now we have a positive or negative number of seconds between the input time and 1970-01-01 00:00:00

	if (utctime_ext(&epoch, bdt)) return epoch;                             //Set if possible all fields so they are in their proper ranges
	else return -1;
}

static struct utctm timedata;

struct utctm* utctime(const Bit64s* a)
{
	//224
	return utctime_ext(a, &timedata);
}

struct utctm* pushtm(struct tm* src)
{
	timedata.tm_sec = src->tm_sec;
	timedata.tm_min = src->tm_min;
	timedata.tm_hour = src->tm_hour;
	timedata.tm_wday = src->tm_wday;
	timedata.tm_yday = src->tm_yday;
	timedata.tm_mday = src->tm_mday;
	timedata.tm_mon = src->tm_mon;
	timedata.tm_year = src->tm_year;
	return &timedata;
}