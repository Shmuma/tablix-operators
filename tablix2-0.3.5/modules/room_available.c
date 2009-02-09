/* TABLIX, PGA general timetable solver                              */
/* Copyright (C) 2002-2005 Tomaz Solc                                      */

/* This program is free software; you can redistribute it and/or modify    */
/* it under the terms of the GNU General Public License as published by    */
/* the Free Software Foundation; either version 2 of the License, or       */
/* (at your option) any later version.                                     */

/* This program is distributed in the hope that it will be useful,         */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/* GNU General Public License for more details.                            */

/* You should have received a copy of the GNU General Public License       */
/* along with this program; if not, write to the Free Software             */
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

/* $Id: available.c,v 1.1 2006-05-15 19:57:11 avian Exp $ */

/** @module
 *
 * @author Max Lapam
 * @author-email max.lapan@gmail.com
 *
 * @brief Specifies periods of availability of given room.
 *
 * @ingroup DC Scheduling
 */

/** @resource-restriction available
 *
 * <restriction type="room-not-available-KIND">NUM</restriction>
 *
 * This tuple restriction specifies that the room resouce is available at time slot at the specified
 * period and (optional) day. If room have no available restrictions, this mean that this room is available at all
 * defined timeslots.
 *
 * For example:
 *
 * <resourcetype type="room">
 * 	<resource name="A">
 * 		<restriction type="room-not-available-time">timeslot</restriction>
 * 		<restriction type="room-not-available-day">day</restriction>
 * 		<restriction type="room-not-available-cell">day time</restriction>
 * 	</resource>
 * </resource>
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "module.h"

#define RESTR_TIME "room-not-available-time"
#define RESTR_DAY  "room-not-available-day"
#define RESTR_CELL "room-not-available-cell"

static resourcetype *time;
static resourcetype *room;
static int periods, days;

static char** rooms_timeslots;


int not_available(char *restriction, char *cont, resource *res)
{
    int n, m, err = 1, i, id;

    if (!strcmp (restriction, RESTR_TIME)) {
        /* handle timeslot restriction for given room */
        if (sscanf (cont, "%d", &n)) {
            err = 0;
            for (i = 0; i < days; i++)
                rooms_timeslots[res->resid][n + i * periods] = 0;
        }
    }
    else if (!strcmp (restriction, RESTR_DAY)) {
        /* handle day restriction */
        if (sscanf (cont, "%d", &n)) {
            err = 0;
            for (i = 0; i < periods; i++)
                rooms_timeslots[res->resid][i + n * periods] = 0;
        }
    }
    else if (!strcmp (restriction, RESTR_CELL)) {
        id = res_findid (time, cont);

        /* handle single slot restriction */
        if (id >= 0) {
            err = 0;
            rooms_timeslots[res->resid][id] = 0;
        }
    }

    if (err) {
        error ("Invalid restriction '%s' used in room resource", cont);
        return -1;
    }

    return 0;
}


int fitness (chromo **c, ext **e, slist **s)
{
    chromo *room_chr = c[0];
    chromo *time_chr = c[1];
    int i, sum = 0;

    for (i = 0; i < room_chr->gennum; i++) {
        if (!rooms_timeslots[room_chr->gen[i]][time_chr->gen[i]])
            sum++;
    }

    return sum;
}


int module_precalc (moduleoption *opt)
{
    int i, j, k;
    char buf[1024];

    for (i = 0; i < room->resnum; i++) {
        info ("Room %d:", i);
        buf[days] = 0;
       
        for (j = 0; j < periods; j++) {
            for (k = 0; k < days; k++)
                buf[k] = rooms_timeslots[i][k*periods + j] ? '*' : '_';
            info ("%s", buf);
        }

        info ("");
    }
    return 0;
}


int module_init(moduleoption *opt) 
{
    int res, i;
    fitnessfunc *f;

    precalc_new (module_precalc);

    time = restype_find("time");
    if (time == NULL)
        return -1;

    room = restype_find("room");
    if (room == NULL)
        return -1;

    res = res_get_matrix (time, &days, &periods);
    if (res) {
        error ("Resource 'time' is not a matrix");
        return -1;
    }

    info ("Time is %d x %d, Rooms is %d", days, periods, room->resnum);
    
    rooms_timeslots = (char**)malloc (sizeof (char*) * room->resnum);

    for (i = 0; i < room->resnum; i++) {
        rooms_timeslots[i] = (char*)malloc (days * periods);
        memset (rooms_timeslots[i], 1, days * periods);
    }

    handler_res_new("room", RESTR_TIME, not_available);
    handler_res_new("room", RESTR_DAY,  not_available);
    handler_res_new("room", RESTR_CELL, not_available);

    f = fitness_new ("rooms-available",
                     option_int (opt, "weight"),
                     option_int (opt, "mandatory"),
                     fitness);

    fitness_request_chromo (f, "room");
    fitness_request_chromo (f, "time");

    return(0);
}
