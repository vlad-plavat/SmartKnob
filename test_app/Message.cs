using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.ConstrainedExecution;
using HidLibrary;
using UnityEngine;

public class Message : MonoBehaviour
{
    public static byte MESSAGE_NOP = 0;
    public static byte MESSAGE_LED = 1;
    public static byte MESSAGE_LCD = 2;
    public static byte MESSAGE_MOT = 3;
    public static byte MOTOR_OFF = 2;
    public static byte MOTOR_VEL = 1;
    public static HidReport MessageNOP(){
        HidReport report = new HidReport(65);
        report.Data[0] = MESSAGE_NOP;
        return report;
    }
    public static HidReport MessageLED(Color32[] colors, bool rotating){
        HidReport report = new HidReport(65);
        int reportIndex = 0;
        report.Data[reportIndex++] = MESSAGE_LED;
        for(int i=0; i<16; i++){
            Color32 c = colors[i];
            report.Data[reportIndex++] = c.g;
            report.Data[reportIndex++] = c.r;
            report.Data[reportIndex++] = c.b;
        }
        report.Data[reportIndex++] = rotating?(byte)0x01:(byte)0x00;
        return report;
    }
    public static HidReport MessageMOT(bool vibrate, byte mode){
        HidReport report = new HidReport(65);
        int reportIndex = 0;
        report.Data[reportIndex++] = MESSAGE_MOT;
        report.Data[reportIndex++] = vibrate?(byte)0x01:(byte)0x00;
        report.Data[reportIndex++] = mode;
        
        return report;
    }

    
public static byte START_EDIT = 0x01;
public static byte SUBMIT_LIST = 0x02;

public static byte DRAW_RECTANGLE = 0x03;
public static byte GRADIENT_H = 0x04;
public static byte GRADIENT_V = 0x04;
public static byte DRAW_IMAGE = 0x05;
public static byte ROTATED_SCALED_IMAGE = 0x06;
public static byte ROTATED_IMAGE = 0x07;
public static byte DRAW_LINE = 0x08;
public static byte DRAW_CIRCLE = 0x09;
public static byte DRAW_CIRCLE_PART = 0x0A;
public static byte DRAW_LINE_ROUNDED = 0x0B;
public static byte FILL_SCREEN = 0x0C;
public static byte PRINT_LINE = 0x0D;
public static byte PRINT_LINE_BIG = 0x0E;

public static byte WRITE_STRING = 0x10;


    public static List<HidReport> MessageLCD(Color32 background){
        List<HidReport> report_list = new List<HidReport>();
        HidReport report = new HidReport(65);
        report.Data[0] = MESSAGE_LCD;
        report.Data[1] = START_EDIT;
        report.Data[2] = SUBMIT_LIST;

        Int32[] commands = new Int32[15];
        commands[0] = FILL_SCREEN;
        UInt16 color = (UInt16)(((UInt16)background.r << 11) | ((UInt16)background.g << 5) | (UInt16)background.b);
        commands[1] = color;
        Buffer.BlockCopy(commands,0,report.Data,4, 60);

        report_list.Add(report);
        
        return report_list;
    }


}
