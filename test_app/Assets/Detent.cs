using System.Collections;
using System.Collections.Generic;
using UnityEngine.EventSystems;
using UnityEngine;
using UnityEngine.UI;
using Unity.Rendering.HybridV2;

public class Detent : MonoBehaviour, IPointerDownHandler, IPointerUpHandler
{
    public bool buttonPressed = false;
    public float angle = 0;
    public void OnPointerDown(PointerEventData eventData){
        buttonPressed = true;
    }
    
    public void OnPointerUp(PointerEventData eventData){
        buttonPressed = false;
    }
    void Start(){
        Vector3 dif = gameObject.transform.position - gameObject.transform.parent.position;
        Vector2 dif2d = new Vector2(dif.x, dif.y);
        angle = -Vector2.SignedAngle(Vector2.up, dif2d);
        dif.Normalize();
        dif *= 100;
        gameObject.transform.localPosition = dif;
    }

    // Update is called once per frame
    void Update()
    {
        if(buttonPressed){
            Vector3 dif = Input.mousePosition - gameObject.transform.parent.position;
            Vector2 dif2d = new Vector2(dif.x, dif.y);
            angle = -Vector2.SignedAngle(Vector2.up, dif2d);
            dif.Normalize();
            dif *= 100;
            gameObject.transform.localPosition = dif;
        }
    }
}
