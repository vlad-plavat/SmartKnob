using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.ConstrainedExecution;
using System.Runtime.InteropServices;
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

    static System.Random random = new System.Random();

    public static void addElement(Int32 elem, byte[] array, int i){
        array[i*4]   = (byte)(elem&0xFF);
        array[i*4+1] = (byte)((elem>>8)&0xFF);
        array[i*4+2] = (byte)((elem>>16)&0xFF);
        array[i*4+3] = (byte)((elem>>24)&0xFF);
    }

    public static List<HidReport> MessageLCD(Color32 background, Int32 r){
        float angle = 360f-r*360f/1024/16;
        List<HidReport> report_list = new List<HidReport>();

        List<Int32> allcommands = new List<Int32>{START_EDIT, 
        FILL_SCREEN, 
            0xF000,0,0,0,0,0,0,0,
        DRAW_CIRCLE, 
            120+Mathf.RoundToInt(20*Mathf.Sin(-angle*Mathf.Deg2Rad)),
            120+Mathf.RoundToInt(20*Mathf.Cos(-angle*Mathf.Deg2Rad)),
            10<<16, 0xF0F0, 0,0,0,0,
        
        SUBMIT_LIST};

        int nrSet=15;
        HidReport report = null;//new HidReport(65)
        foreach(Int32 cr in allcommands){
            if(nrSet == 15){
                if(report != null){
                    report.Data[1] = (byte)nrSet;
                    report_list.Add(report);
                }
                report = new HidReport(65);
                nrSet = 0;
                report.Data[0] = MESSAGE_LCD;
            }
            addElement(cr, report.Data, nrSet+1);nrSet++;
        }
        if(report != null && nrSet!=0){
            report.Data[1] = (byte)nrSet;
            report_list.Add(report);
        }

        
        return report_list;
    }


}
