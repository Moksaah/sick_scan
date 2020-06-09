/**
* \file
* \brief Angle Compensation for high accuracy measurement
*
* Copyright (C) 2020 Ing.-Buero Dr. Michael Lehning, Hildesheim
* Copyright (C) 2020 SICK AG, Waldkirch
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*       http://www.apache.org/licenses/LICENSE-2.0
*
*   Unless required by applicable law or agreed to in writing, software
*   distributed under the License is distributed on an "AS IS" BASIS,
*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*   See the License for the specific language governing permissions and
*   limitations under the License.
*
*
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of Osnabrück University nor the names of its
*       contributors may be used to endorse or promote products derived from
*       this software without specific prior written permission.
*     * Neither the name of SICK AG nor the names of its
*       contributors may be used to endorse or promote products derived from
*       this software without specific prior written permission
*     * Neither the name of Ing.-Buero Dr. Michael Lehning nor the names of its
*       contributors may be used to endorse or promote products derived from
*       this software without specific prior written permission
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*  Last modified: 9th June 2020
*
*      Authors:
*         Michael Lehning <michael.lehning@lehning.de>
*
*
*/

#include "sick_scan/helper/angle_compensator.h"
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <math.h>


using namespace std;

/***************************************************************************************
 *
 * Background information is given in the Markdown file doc/angular_compensation.md
 *
 *
 * The angle compensation compensates the given raw angle by using the formula
 * compensated angle = [raw angle] + [ampl] * sin([raw_angle] + [phase_corr]) + [offset]
 * The offset compensates a small offset deviation in [deg]
 * The sine-wave compensation allows a compensation of a sine wave modulated deviation
 * over 360 deg rotation
 *
 * Details:
 *
 *                |      xxxxx                           xxxxx
 * .[phase_corr]. |    xx  ^  xx                       xx     xx
 *                |  xx    |    xx                   xx         xx
 *                | x      |      x                 x             x
 * x               x       |       x               x               x
 *  x             x      -----      x             x                 x
 *   xx         xx       [ampl]      xx         xx                   xx         xx
 *    xx     xx                       xx     xx                       xx     xx
 *      xxxxx                           xxxxx                           xxxxx
 *
 *  DC-Offset in [deg] corresponds to [offset]
 *
 ***************************************************************************************
 */


/*!
\brief Compensate raw angle given in [RAD]
\param angleInRad: raw angle in [RAD]
*/
double AngleCompensator::compensateAngleInRad(double angleInRad)
{
  double deg2radFactor = 0.01745329252; // pi/180 deg - see for example: https://www.rapidtables.com/convert/number/degrees-to-radians.html
  double angleCompInRad = angleInRad + deg2radFactor * amplCorr * sin(angleInRad + phaseCorrInRad) + offsetCorrInRad;
  return(angleCompInRad);
}

/*!
\brief Compensate raw angle given in [DEG]
\param angleInDeg: raw angle in [DEG]
*/
double AngleCompensator::compensateAngleInDeg(double angleInDeg)
{
  // AngleComp =AngleRaw + AngleCompAmpl * SIN( AngleRaw + AngleCompPhase ) + AngleCompOffset
  double angleCompInDeg;
  double deg2radFactor = 0.01745329252; // pi/180 deg - see for example: https://www.rapidtables.com/convert/number/degrees-to-radians.html
  double angleRawInRad = deg2radFactor * angleInDeg;
  double phaseCorrInRad= deg2radFactor * phaseCorrInDeg;
  angleCompInDeg = angleInDeg + amplCorr * sin(angleRawInRad + phaseCorrInRad) + offsetCorrInDeg;
  return(angleCompInDeg);
}

/*!
\brief Parse ASCII reply
\param replyStr holds reply with the angular compensation information
*/
int AngleCompensator::parseAsciiReply(const char *replyStr)
{
  int retCode = 0;
  std::stringstream ss(replyStr);
  std::string token;
  std::vector<std::string> cont;
  cont.clear();
  char delim = ' ';
  while (std::getline(ss, token, delim))
  {
    cont.push_back(token);
  }


  // if (val > 32767) val -= 65536;

  std::string s = "fffefffe";
  int x = std::stoul(s, nullptr, 16);

  // sizes in bit of the following values
  // 16
  // 32
  // 16

  int16_t ampl10000th;
  int32_t phase10000th;
  int16_t offset10000th;


  if (cont.size() == 5)
  {
    unsigned long helperArr[3] = {0};
    for (int i = 0; i < 3; i++)
    {
      char ch = cont [2+i][0];
      if ((ch == '+') || (ch == '-'))
      {
        sscanf(cont[2+i].c_str(),"%ld", &(helperArr[i]));
      }
      else
      {
         helperArr[i] = std::stoul(cont[2+i], nullptr, 16);
      }
    }
    ampl10000th = (int16_t)(0xFFFF & helperArr[0]);
    phase10000th = (int32_t)(0xFFFFFFFF & helperArr[1]); // check againt https://www.rapidtables.com/convert/number/hex-to-decimal.html
    offset10000th = (int16_t)(0xFFFF & helperArr[2]);
  }

  amplCorr = 1/10000.0 * ampl10000th;
  phaseCorrInDeg = 1/10000.0 * phase10000th;
  offsetCorrInDeg = 1/10000.0 * offset10000th;

  offsetCorrInRad = offsetCorrInDeg/180.0 * M_PI;
  phaseCorrInRad = phaseCorrInDeg/180.0 * M_PI;
  return (retCode);
}

/*!
\brief Parse reply of angle compensation values given the command MCAngleCompSin (see testbed)
\param isBinary: reply is in binary format (true) or in ASCII format (false)
\param replyVec holds received byte array
*/
int AngleCompensator::parseReply(bool isBinary, std::vector<unsigned char>& replyVec)
{
  int retCode = 0;
std::string stmp;
  if (isBinary) // convert binary message into the ASCII format to reuse parsing algorithm
  {
    stmp = "";
    int sLen = replyVec.size();
    int offset = sLen - 12;
    int relCnt = 0;
    for (int i = 0; i < sLen; i++)
    {
      if (i < offset)
      {
        stmp += (char)replyVec[i];
      }
      else
      {
        char szTmp[3];
        sprintf(szTmp,"%02X", replyVec[i]);
        relCnt++;
        stmp += szTmp;
        if (relCnt < 12)
        {
          if ((relCnt % 4) == 0)
          {
             stmp += ' ';
          }
        }

      }
    }
  }
  parseAsciiReply(stmp.c_str());
  return(retCode);
}

/*!
\brief Testbed for angle compensation
*/
void AngleCompensator::testbed()
{
  AngleCompensator ac;
  std::vector<unsigned char> testVec;

  std::string s = string("sRA MCAngleCompSin ");
  for (int i = 0; i < s.length(); i++)
  {
    testVec.push_back(s[i]);
  }
  unsigned char dataArr[] = {0x00,0x00,0x07,0x65,0xff,0xfc,0xc9,0xb9,0xff,0xff,0xff,0x0b };
  for (int i = 0; i < sizeof(dataArr)/sizeof(dataArr[0]); i++)
  {
    testVec.push_back(dataArr[i]);
  }
  ac.parseReply(true,testVec);


  testVec.clear();
  s = "sRA MCAngleCompSin 765 FFFCC9B9 FFFFFF0B";
  for (int i = 0; i < s.length(); i++)
  {
    testVec.push_back(s[i]);
  }
  ac.parseReply(false,testVec);
  ac.parseAsciiReply("sRA MCAngleCompSin 765 FFFCC9B9 FFFFFF0B");
  ac.parseAsciiReply("sRA MCAngleCompSin +1893 -210503 -245");
  FILE *fout = fopen("angle_compensation_debug.csv","w");
  fprintf(fout,"Input   ;Output  ;Correction\n");
  for (int i =0; i <= 359; i++)
  {
    double rawAngle = (double)i;
    double compAngle = ac.compensateAngleInDeg(rawAngle);
    double compRadAngle = ac.compensateAngleInRad(rawAngle/180.0*M_PI);
    double checkAngle = compRadAngle/M_PI*180.0;
    fprintf(fout,"%10.6lf;%10.6lf;%10.6lf\n", rawAngle, compAngle, compAngle - rawAngle);
  }
  fclose(fout);
}