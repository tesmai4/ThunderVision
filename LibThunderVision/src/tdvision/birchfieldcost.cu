#include <cuda_runtime.h>
#include <cuda.h>
#include "cuerr.hpp"
#include "cudaconstraits.hpp"
#include "dsimemutil.h"

#define min3(a, b, c) min(a, min(b, c))
#define max3(a, b, c) max(a, max(b, c))

#define MAX_LINE_SIZE 1024
#define BT_N 4

texture<float, 2> texLeftImg;
texture<float, 2> texRightImg;

__global__ void birchfieldKern(const DSIDim dsiDim, const int maxDisparity, 
                               float *dsiMem)
{
  const uint x = threadIdx.x;
  const uint y = blockIdx.x;
  
  __shared__ float leftScanLine[MAX_LINE_SIZE + 2];
  __shared__ float rightScanLine[MAX_LINE_SIZE + 2];
  
  leftScanLine[x] = tex2D(texLeftImg, x, y);
  rightScanLine[x] = tex2D(texRightImg, x, y);
  
  if ( x == 0 ) {
    leftScanLine[0] = 0.0f;
    rightScanLine[0] = 0.0f;
  }
  
  __syncthreads();
    
  for (int disp=0; disp < maxDisparity; disp++) {   
    float costValue = CUDART_INF_F;
    
    if ( static_cast<int>(x) - disp >= 0 ) {       
      costValue = 0.0f;
      
      const uint sx = x + 1;
      
      for (uint v=sx; v < sx + BT_N; v++) {  
        const uint vd = v - disp;
          
        const float lI = leftScanLine[v];
        const float rI = rightScanLine[vd];  
      
        const float laI = 0.5f*(lI + leftScanLine[v - 1]);
        const float lbI = 0.5f*(lI + leftScanLine[v + 1]);
  
        const float raI = 0.5f*(rI + rightScanLine[vd - 1]);
        const float rbI = 0.5f*(rI + rightScanLine[vd + 1]);

        const float lImi = min3(laI, lbI, lI);
        const float lIma = max3(laI, lbI, lI);

        const float rImi = min3(raI, rbI, rI);
        const float rIma = max3(raI, rbI, rI);
    
        costValue += min(max3(0.0f, lI - rIma, rImi - lI),
                         max3(0.0f, rI - lIma, lImi - rI));
              
      }            
    }
    
    dsiSetIntensity(dsiDim, x, y, disp, costValue, dsiMem);
  }          
}

TDV_NAMESPACE_BEGIN

void BirchfieldCostRun(int maxDisparity,
                       Dim dsiDim, float *leftImg_d, float *rightImg_d,
                       float *dsiMem)
{
  CUerrExp err;
    
  err << cudaBindTexture2D(NULL, texLeftImg, leftImg_d, 
                           cudaCreateChannelDesc<float>(),
                           dsiDim.width(), dsiDim.height(),
                           dsiDim.width()*sizeof(float));
  
  err << cudaBindTexture2D(NULL, texRightImg, rightImg_d, 
                           cudaCreateChannelDesc<float>(),
                           dsiDim.width(), dsiDim.height(),
                           dsiDim.width()*sizeof(float));
  
  texLeftImg.addressMode[0] = texRightImg.addressMode[0] = cudaAddressModeWrap;
  texLeftImg.addressMode[1] = texRightImg.addressMode[1] = cudaAddressModeWrap;
  texLeftImg.normalized = texRightImg.normalized = false;
  texLeftImg.filterMode = texRightImg.filterMode = cudaFilterModePoint;
    
  DSIDim ddim(DSIDimCreate(dsiDim));      
  
  birchfieldKern<<<ddim.y, ddim.x>>>(ddim, maxDisparity, dsiMem); 

  cudaThreadSynchronize();
}

TDV_NAMESPACE_END