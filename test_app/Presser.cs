using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class Presser : MonoBehaviour
{
    float grad = 1;

    public void Press(){
        grad=0.5f;
    }

    public float getGrad(){
        return 2f-2f*grad;
    }

    // Update is called once per frame
    void Update()
    {
        gameObject.GetComponent<RawImage>().color = Color.Lerp(Color.black, Color.white, grad);
        grad = grad+1f*Time.deltaTime;
        if(grad>1f)grad=1f;
    }
}
