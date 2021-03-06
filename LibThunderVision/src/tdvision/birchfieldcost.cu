#include <cuda_runtime.h>
#include <cuda.h>
#include <math_constants.h>
#include "cuerr.hpp"
#include "cudaconstraits.hpp"
#include "dsimemutil.h"

texture<float, 2> texLeftImg;
texture<float, 2> texRightImg;

#define min3(a, b, c) min(a, min(b, c))
#define max3(a, b, c) max(a, max(b, c))

__device__ float costAtDisp(int x, int y, int disp)
{
  float sum = 0.0f;

  for (int v=x; v < x + 1; v++) {
    const float lI = tex2D(texLeftImg, v, y);
    const float rI = tex2D(texRightImg, v - disp, y);

    const float laI = 0.5f*(lI + tex2D(texLeftImg, v - 1, y));
    const float lbI = 0.5f*(lI + tex2D(texLeftImg, v + 1, y));

    const float raI = 0.5f*(rI + tex2D(texRightImg, v - disp - 1, y));
    const float rbI = 0.5f*(rI + tex2D(texRightImg, v - disp + 1, y));

    const float lImi = min3(laI, lbI, lI);
    const float lIma = max3(laI, lbI, lI);

    const float rImi = min3(raI, rbI, rI);
    const float rIma = max3(raI, rbI, rI);

    sum += min(max3(0.0f, lI - rIma, rImi - lI),
               max3(0.0f, rI - lIma, lImi - rI));
  }

  return sum;
}

__global__ void birchfieldKernTexture(const dim3 dsiDim, cudaPitchedPtr costDSI)
{
  int x = blockIdx.x*blockDim.x + threadIdx.x;
  int y = blockIdx.y*blockDim.y + threadIdx.y;

  if ( x < dsiDim.x && y < dsiDim.y ) {
    float *costDsiRow = dsiGetRow(costDSI, dsiDim.y, x, y);

    for (int disp=0; disp < dsiDim.z; disp++) {
      float cost = CUDART_INF_F;

      if ( x - disp >= 0 ) {
        cost = costAtDisp(x, y, disp);
      }

      costDsiRow[disp] = cost;
    }
  }
}

#define min3(a, b, c) min(a, min(b, c))
#define max3(a, b, c) max(a, max(b, c))

#define MAX_LINE_SIZE 1024
#define BT_LEFT_N 4
#define BT_RIGHT_N 4

__global__ void birchfieldKernSharedMem(const dim3 dsiDim,
                                        const float *leftImg, const float *rightImg,
                                        cudaPitchedPtr costDSI)
{
  const short x = threadIdx.x;
  const ushort y = blockIdx.x;

  __shared__ float leftScanLine[MAX_LINE_SIZE + 2];
  __shared__ float rightScanLine[MAX_LINE_SIZE + 2];

  const uint cPixelOffset = y*dsiDim.x + x;

  const short nx = x + 1;
    
  leftScanLine[nx] = leftImg[cPixelOffset];
  rightScanLine[nx] = rightImg[cPixelOffset];

  if ( x == 0 ) {
    leftScanLine[0] = 0.0f;
    rightScanLine[0] = 0.0f;

    leftScanLine[dsiDim.x + 1] = 0.0f;
    rightScanLine[dsiDim.x + 1] = 0.0f;
  }

  __syncthreads();

  float *costDsiRow = dsiGetRow(costDSI, dsiDim.y, x, y);

  const short nDisps = min(dsiDim.z, x + 1);
  for (short disp=0; disp < nDisps; disp++) {
    const short start = max(0, x - disp - BT_LEFT_N) + disp;
    const short end = min(dsiDim.x, x + BT_RIGHT_N);

    float costValue = 0.0f;

    for (ushort p=start; p < end; p++) {
      const ushort lIdx = p + 1;
      const ushort rIdx = p - disp + 1;

      const float lI = leftScanLine[lIdx];
      const float rI = rightScanLine[rIdx];

#if 1
      const float laI = 0.5f*(lI + leftScanLine[lIdx - 1]);
      const float lbI = 0.5f*(lI + leftScanLine[lIdx + 1]);

      const float raI = 0.5f*(rI + rightScanLine[rIdx - 1]);
      const float rbI = 0.5f*(rI + rightScanLine[rIdx + 1]);
#else
      const float laI = 0.5f*(lI + leftScanLine[lIdx]);
      const float lbI = 0.5f*(lI + leftScanLine[lIdx]);

      const float raI = 0.5f*(rI + rightScanLine[rIdx]);
      const float rbI = 0.5f*(rI + rightScanLine[rIdx]);

#endif
      const float lImi = min3(laI, lbI, lI);
      const float lIma = max3(laI, lbI, lI);

      const float rImi = min3(raI, rbI, rI);
      const float rIma = max3(raI, rbI, rI);

      costValue += min(max3(0.0f, lI - rIma, rImi - lI),
                       max3(0.0f, rI - lIma, lImi - rI));
    }

    costDsiRow[disp] = costValue;
  }

  for (short disp=nDisps; disp<dsiDim.z; disp++) {
    costDsiRow[disp] = CUDART_INF_F;
  }
}

TDV_NAMESPACE_BEGIN

static void TextureBirchfieldRun(Dim dsiDim,
                                 float *leftImg_d, float *rightImg_d,
                                 cudaPitchedPtr dsiMem)
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

  CudaConstraits constraits;
  WorkSize ws = constraits.imageWorkSize(dsiDim);
  birchfieldKernTexture<<<ws.blocks, ws.threads>>>(tdvDimTo(dsiDim), dsiMem);
}

static void SharedMemBirchfieldRun(Dim dsiDim,
                                   float *leftImg_d, float *rightImg_d,
                                   cudaPitchedPtr dsiMem)
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

  birchfieldKernSharedMem<<<dsiDim.height(),
    dsiDim.width()>>>(tdvDimTo(dsiDim), leftImg_d, rightImg_d, dsiMem);
}

void BirchfieldCostRun(Dim dsiDim,
                       float *leftImg_d, float *rightImg_d,
                       cudaPitchedPtr costDSI)
{
    cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, 0);

    //    if ( static_cast<int>(dsiDim.width()) <= prop.maxThreadsPerBlock ) {
    if ( false ) {
      SharedMemBirchfieldRun(dsiDim, leftImg_d, rightImg_d,
                             costDSI);
    }
    else {
      TextureBirchfieldRun(dsiDim, leftImg_d, rightImg_d,
                           costDSI);
    }
    CUerrExp err;
    err << cudaThreadSynchronize();
}

TDV_NAMESPACE_END
