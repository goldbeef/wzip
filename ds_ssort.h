/* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
   Prototypes for the Deep Shallow Suffix Sort routines
   >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> */ 
#ifndef _DS_SSORT_H

/*Copyright (C) 2002 Giovanni Manzini

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  Giovanni Manzini
  manzini@mfn.unipmn.it
*/
#define _DS_SSORT_H

#ifdef __cplusplus
extern "C"
{
void ds_ssort(unsigned char *t, unsigned long *sa, long n);
int init_ds_ssort(int adist, int bs_ratio);
}
#endif

void ds_ssort(unsigned char *t,unsigned long *sa,long n);
int init_ds_ssort(int adlist,int bs_ratio);

#endif 
