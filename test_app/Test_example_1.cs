using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;
using System.Runtime.InteropServices;
using UnityEngine.UI;

//using UnityEngine.Experimental;
//using UnityEngine.Experimental.Input;


 
using HidLibrary;
using System.Linq;
using System.Threading.Tasks;
using System.Data;
using UnityEngine.Audio;
public class Test_example_1 : MonoBehaviour
{

    public GameObject obj;

    static HidDevice smartKnob = null;
    static bool ended = false;
    static float crTime;
    static bool vibrate = false;
    public byte g;
    public AudioClip kick, clap, snare, hihat;
    public AudioSource beatsAudioSource, scratchAudioSource, flangeAudioSource;
    public AudioMixer audioMixer;

    static HidReport reportIn = null;
    static Queue<HidReport> writeQueue = new Queue<HidReport>();

    GameObject vinyl, gkick, gsnare, ghihat, gclap;
    void Start(){
        //Debug.Log(HidDevices.Enumerate().Count<HidDevice>());
        vinyl = GameObject.Find("Vinyl");
        gkick = GameObject.Find("Kick");
        gsnare = GameObject.Find("Snare");
        ghihat = GameObject.Find("Hihat");
        gclap = GameObject.Find("Clap");
        
        foreach(HidDevice d in HidDevices.Enumerate()){
            //Debug.Log(d);
            if(d.Attributes.VendorId == 0xCAFE && d.Attributes.ProductId == 0x4005){
                smartKnob = d;
            }
        }

        if(smartKnob == null){
            Debug.LogError("Device not found!");
        }

        //overlap?????
        smartKnob.OpenDevice();
        if(!smartKnob.IsOpen){
            Debug.LogError("Device not open!");
        }


        /*try {
            //byte[] data = new byte[32];
            Task<HidReport> t = smartKnob.ReadReportAsync(100);

            int timeout = 3000;
            if (await Task.WhenAny(t, Task.Delay(timeout)) == t) {
                // task completed within timeout
                Debug.Log(t.Result);
            } else { 
                // timeout logic
                Debug.Log("yoink");
            }
        }catch{
            Debug.Log("error");
        }*/

        smartKnob.ReadReport(OnReport);
        HidReport rep = new HidReport(65);
        for(int i=0;i<64;i++)rep.Data[i]=0;
        smartKnob.WriteReport(rep, OnReportW);
        //HidDeviceData data = smartKnob.Read(10);
        //Debug.Log(data);
        StartBeating();

    }

    void OnApplicationQuit(){
        smartKnob.CloseDevice();
        ended = true;
        if(smartKnob.IsOpen){
            Debug.LogError("Device not closed!");
        }
    }

    private static void OnReport(HidReport report){
        //Debug.Log(report.GetBytes().Count() + " " + string.Join(",", report.GetBytes()));
        reportIn = report;
        if(!ended)
            smartKnob.ReadReport(OnReport);
    }
    
    private static void OnReportW(bool success){
        if(writeQueue.Count() == 0) return;
        HidReport rep = writeQueue.Dequeue();
        if(!ended)
            smartKnob.WriteReport(rep, OnReportW);
    }

    private static void SendReport(HidReport rep){
        if(writeQueue.Count()==0){
            smartKnob.WriteReport(rep, OnReportW);
        }else{
            writeQueue.Enqueue(rep);
        }
    }
    float[] smoother = new float[4];
    float nextSmooth(float v){
        for(int i=0; i<smoother.Count()-1; i++){
            smoother[i] = smoother[i+1];
        }
        smoother[smoother.Count()-1] = v;
        float s=0;
        for(int i=0; i<smoother.Count(); i++){
            s += smoother[i];
        }
        return s/smoother.Count();
    }

    int CheckBeat(int var, int dir, int prev, AudioClip clip, GameObject g){
        if(var*dir>1500 && prev == 0){
            prev = 1;
            beatsAudioSource.PlayOneShot(clip);
            if(beatsAudioSource.mute == false)
                g.GetComponent<Presser>().Press();
        }else if(var*dir<750 && prev == 1){
            prev = 0;
        }
        return prev;
    }
    int prevKick = 0, prevClap = 0, prevHat = 0, prevSnare = 0;
    float prevAngle = 0;
    float prevLEDservice = 0;
    void Update(){
        crTime = Time.timeSinceLevelLoad;
        if(crTime - prevLEDservice > 0.03){
            prevLEDservice = crTime;
            if(beatsAudioSource.mute == false){
                Color32[] color_arr = new Color32[16];
                for(int i=7;i<=9;i++){
                    byte level = (byte)(gkick.GetComponent<Presser>().getGrad()*255.0f);
                    color_arr[i] = new Color32(level,level,0,0);
                }
                for(int i=11;i<=13;i++){
                    byte level = (byte)(gsnare.GetComponent<Presser>().getGrad()*255.0f);
                    color_arr[i] = new Color32(level,level,0,0);
                }
                for(int i=3;i<=5;i++){
                    byte level = (byte)(ghihat.GetComponent<Presser>().getGrad()*255.0f);
                    color_arr[i] = new Color32(level,level,0,0);
                }
                
                byte level2 = (byte)(gclap.GetComponent<Presser>().getGrad()*255.0f);
                color_arr[0] = color_arr[1] = color_arr[15] = new Color32(level2,level2,0,0);
                
                HidReport rep = Message.MessageLED(color_arr, false);
                SendReport(rep);
            }
            if(flangeAudioSource.mute == false){
                Color32[] color_arr = new Color32[16];
                for(int i=0;i<16;i++)
                    color_arr[i] = new Color32(255,0,255,0);
                HidReport rep = Message.MessageLED(color_arr, false);
                SendReport(rep);
            }

            List<HidReport> reports = Message.MessageLCD(new Color32(255,0,0,0));
            foreach(HidReport hidReport in reports){
                SendReport(hidReport);
            }
        }

        if(reportIn == null) return;
        byte[] intarray = new byte[4];
        Array.Copy(reportIn.Data, 0, intarray, 0, 4);
        Int32 Xtilt = BitConverter.ToInt32(intarray, 0);
        Array.Copy(reportIn.Data, 4, intarray, 0, 4);
        Int32 Ytilt = BitConverter.ToInt32(intarray, 0);
        Array.Copy(reportIn.Data, 8, intarray, 0, 4);
        Int32 Press = BitConverter.ToInt32(intarray, 0);
        Array.Copy(reportIn.Data, 12, intarray, 0, 4);
        Int32 Rotation = BitConverter.ToInt32(intarray, 0);
        Array.Copy(reportIn.Data, 16, intarray, 0, 4);
        Int32 Speed = BitConverter.ToInt32(intarray, 0);
        obj.transform.rotation = Quaternion.Euler(new Vector3(Ytilt/100,0,Xtilt/100));
        prevKick = CheckBeat(Ytilt,1,prevKick,kick,gkick);
        prevClap = CheckBeat(Ytilt,-1,prevClap,clap,gclap);
        prevHat  = CheckBeat(Xtilt,1,prevHat,hihat,ghihat);
        prevSnare  = CheckBeat(Xtilt,-1,prevSnare,snare,gsnare);
        float angle = 360f-Rotation*360f/1024/16;
        vinyl.transform.rotation = Quaternion.Euler(new Vector3(0,0,-angle));
        audioMixer.SetFloat("lowpass", (Mathf.Sin(angle*Mathf.Deg2Rad)+1)*1000+100);
        audioMixer.SetFloat("highpass", (Mathf.Sin(angle*Mathf.Deg2Rad)+1)*1000+100);
       
        float vel = Speed;
        vel /= 180000;
        vel /= 2;
        if(Mathf.Abs(vel) < 10){
            vel = nextSmooth(vel);
            if(Mathf.Abs(vel-1)<0.2)
                vel=1;
            scratchAudioSource.pitch = vel;
        }
        //scratchAudioSource.volume = Mathf.Clamp(1f*Press/5000,0,1);
        prevAngle = angle;

    }
    void FixedUpdate()
    {
    }

    public void StartBeating(){
        HidReport rep = Message.MessageMOT(true, Message.MOTOR_OFF);
        SendReport(rep);
        scratchAudioSource.mute = true;
        flangeAudioSource.mute = true;
        beatsAudioSource.mute = false;
    }

    public void StartFlanging(){
        HidReport rep = Message.MessageMOT(true, Message.MOTOR_OFF);
        SendReport(rep);
        scratchAudioSource.mute = true;
        flangeAudioSource.mute = false;
        beatsAudioSource.mute = true;
    }
    public void OnVibrationButton(){
        HidReport rep = Message.MessageMOT(true, 0);
        SendReport(rep);
    }
    public void StartScratching(){
        HidReport rep = Message.MessageMOT(true, Message.MOTOR_VEL);
        SendReport(rep);
        Color32[] color_arr = new Color32[16];
        for(int i=0;i<16;i++){
            byte intensity = (byte)(i*16);
            color_arr[i] = new Color32(intensity,intensity,intensity,0);
        }
        rep = Message.MessageLED(color_arr, true);
        SendReport(rep);
        scratchAudioSource.mute = false;
        flangeAudioSource.mute = true;
        beatsAudioSource.mute = true;
    }

}
