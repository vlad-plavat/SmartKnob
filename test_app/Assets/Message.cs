using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.ConstrainedExecution;
using System.Runtime.InteropServices;
using System.Security.Cryptography.X509Certificates;
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
            report.Data[reportIndex++] = c.r;
            report.Data[reportIndex++] = c.g;
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

    public static HidReport MessageMotDetentList(float[] detents, float offset){
        HidReport report = new HidReport(65);
        int reportIndex = 0;
        report.Data[reportIndex++] = MESSAGE_MOT;
        report.Data[reportIndex++] = 0;//no vibration
        report.Data[reportIndex++] = 5;
        report.Data[reportIndex++] = (byte)detents.Length;
        
        float[] offserarr = new float[]{offset};
        Buffer.BlockCopy(offserarr, 0, report.Data, 4, 4);
        for(int i=0; i<detents.Length; i++){
            Buffer.BlockCopy(detents, i*4, report.Data, (i+2)*4, 4);
        }


        return report;

    }

    
public static byte START_EDIT = 0x01;
public static byte SUBMIT_LIST = 0x02;


public static byte FLAG_START = 0x01;
public static byte FLAG_STOP  = 0x02;

public static byte DRAW_RECTANGLE = 0x03;
public static byte GRADIENT_H = 0x04;
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
public static byte GRADIENT_V = 0x0F;

public static byte WRITE_STRING = 0x10;


enum SmartKImages: uint {SMART_KNOB_IMG=0, CROSSHAIR_CALIB, LEDRING, BRIGHTNESS_IMG, 
            START_APP_IMG, POWER_IMG, MOUSE_IMG, JOYSTICK_IMG, 
            VINYL_IMG, KICK_IMG, SNARE_IMG, HIHAT_IMG, CLAP_IMG, CURSOR_IMG, NUM_PREDEFINED_IMAGES};

    static System.Random random = new System.Random();


    public static void addElement(Int32 elem, byte[] array, int i){
        array[i*4]   = (byte)(elem&0xFF);
        array[i*4+1] = (byte)((elem>>8)&0xFF);
        array[i*4+2] = (byte)((elem>>16)&0xFF);
        array[i*4+3] = (byte)((elem>>24)&0xFF);
    }

    static List<HidReport> MessageLCD(List<Int32> allcommands){
        List<HidReport> report_list = new List<HidReport>();
        int nrSet=15;
        HidReport report = null;//new HidReport(65)
        foreach(Int32 cr in allcommands){
            if(nrSet == 15){
                if(report != null){
                    report.Data[1] = (byte)nrSet;
                    report_list.Add(report);
                }
                report = new HidReport(65);
                report.Data[2] = 0;
                nrSet = 0;
                report.Data[0] = MESSAGE_LCD;
            }
            addElement(cr, report.Data, nrSet+1);nrSet++;
        }
        if(report != null && nrSet!=0){
            report.Data[1] = (byte)nrSet;
            report.Data[2] |= FLAG_STOP;
            report_list.Add(report);
        }

        if(report_list.Count > 0){
            report_list[0].Data[2] |= FLAG_START;
        }

        
        return report_list;
    }

    public static List<HidReport> MessageLCD_Vinyl(Color32 background, Int32 r){
        float angle = 360f-r*360f/1024/16;

        List<Int32> allcommands = new List<Int32>{
        FILL_SCREEN|unchecked((int)0X01000000), 
            0x0000,
        ROTATED_IMAGE|unchecked((int)0X0f000000), 
            120, 120, (int)SmartKImages.VINYL_IMG, Mathf.RoundToInt(angle*64*1024)
        };

        return MessageLCD(allcommands);
    }
    public static List<HidReport> MessageLCD_Beats(){

        List<Int32> allcommands = new List<Int32>{
        FILL_SCREEN|unchecked((int)0X01000000), 
            0x0000,
        DRAW_IMAGE|unchecked((int)0X87000000), 
            120, 50, (int)SmartKImages.KICK_IMG, 1,
        DRAW_IMAGE|unchecked((int)0X87000000), 
            190, 120, (int)SmartKImages.SNARE_IMG, 1,
        DRAW_IMAGE|unchecked((int)0X87000000), 
            50, 120, (int)SmartKImages.HIHAT_IMG, 1,
        DRAW_IMAGE|unchecked((int)0X87000000), 
            120, 190, (int)SmartKImages.CLAP_IMG, 1,
        };

        return MessageLCD(allcommands);
    }
    public static List<HidReport> MessageLCD_Flange(){

        List<Int32> allcommands = new List<Int32>{
        FILL_SCREEN|unchecked((int)0X01000000), 
            0x0000,
        ROTATED_IMAGE|unchecked((int)0X87000000), 
            120, 120, (int)SmartKImages.SMART_KNOB_IMG, 1
        };

        return MessageLCD(allcommands);
    }
    public static List<HidReport> MessageLCD_Motor(float[] detents, float angle){

        List<Int32> allcommands = new List<Int32>{
        FILL_SCREEN|unchecked((int)0X01000000), 
            0x0000,
        DRAW_CIRCLE|unchecked((int)0X0f000000),
            Mathf.RoundToInt(120+75*Mathf.Sin(detents[0]*Mathf.Deg2Rad)),
            Mathf.RoundToInt(120-75*Mathf.Cos(detents[0]*Mathf.Deg2Rad)),
            10<<16, 0xfba0,
        DRAW_CIRCLE|unchecked((int)0X0f000000),
            Mathf.RoundToInt(120+75*Mathf.Sin(detents[1]*Mathf.Deg2Rad)),
            Mathf.RoundToInt(120-75*Mathf.Cos(detents[1]*Mathf.Deg2Rad)),
            10<<16, 0xfba0,
        DRAW_CIRCLE|unchecked((int)0X0f000000),
            Mathf.RoundToInt(120+75*Mathf.Sin(detents[2]*Mathf.Deg2Rad)),
            Mathf.RoundToInt(120-75*Mathf.Cos(detents[2]*Mathf.Deg2Rad)),
            10<<16, 0xfba0,
        DRAW_CIRCLE|unchecked((int)0X0f000000),
            Mathf.RoundToInt(120+75*Mathf.Sin(detents[3]*Mathf.Deg2Rad)),
            Mathf.RoundToInt(120-75*Mathf.Cos(detents[3]*Mathf.Deg2Rad)),
            10<<16, 0xfba0,
        DRAW_CIRCLE|unchecked((int)0X0f000000),
            Mathf.RoundToInt(120+75*Mathf.Sin(detents[4]*Mathf.Deg2Rad)),
            Mathf.RoundToInt(120-75*Mathf.Cos(detents[4]*Mathf.Deg2Rad)),
            10<<16, 0xfba0,
        ROTATED_IMAGE|unchecked((int)0X0f000000), 
            120, 120, (int)SmartKImages.CURSOR_IMG, Mathf.RoundToInt(angle*64*1024)
        };

        return MessageLCD(allcommands);
    }


}
