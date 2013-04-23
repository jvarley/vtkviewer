// VTK Viewer
// Written 2012-2013 Hal Canary <http://cs.unc.edu/~hal>
// Copyright 2012-2013 University of North Carolina at Chapel Hill.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you
// may not use this file except in compliance with the License.  You
// may obtain a copy of the License at
//
//   LICENSE.md in this repository or
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied.  See the License for the specific language governing
// permissions and limitations under the License.

#include "VTKViewer.h"
#include <vtkPolyData.h>
#include <vtkDataSetReader.h>
#include <vtkPolyDataReader.h>
#include <vtkPLYReader.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkRenderWindow.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkColorTransferFunction.h>
#include <vtkPolyDataNormals.h>
#include <vtkPointData.h>
#include <vtkOBJReader.h>
#include <vtkPDBReader.h>
#include <vtkSphereSource.h>
#include <vtkGlyph3D.h>
#include <vtkTubeFilter.h>
#include <vtkAppendPolyData.h>
#include <vtkSTLReader.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkFloatArray.h>
#include <vtkVersion.h>
#if VTK_MAJOR_VERSION <= 5
  #define setInputData(x,y) ((x)->SetInput(y))
  #define addInputData(x,y) ((x)->AddInput(y))
#else
  #define setInputData(x,y) ((x)->SetInputData(y))
  #define addInputData(x,y) ((x)->AddInputData(y))
#endif
#include <vtkCamera.h>

#include <iostream>
using std::cout;

static void ReadPDB(const char * file_name, vtkPolyData * polyData)
{
  vtkSmartPointer<vtkPDBReader> pdb =
    vtkSmartPointer<vtkPDBReader>::New();
  pdb->SetFileName(file_name);
  pdb->SetHBScale(1.0);
  pdb->SetBScale(1.0);
  pdb->Update();

  vtkSmartPointer<vtkSphereSource> sphere =
    vtkSmartPointer<vtkSphereSource>::New();
  sphere->SetCenter(0, 0, 0);
  sphere->SetRadius(1);

  vtkSmartPointer<vtkGlyph3D> glyph =
    vtkSmartPointer<vtkGlyph3D>::New();
  glyph->SetInputConnection(pdb->GetOutputPort());
  glyph->SetSourceConnection(sphere->GetOutputPort());
  glyph->SetOrient(1);
  glyph->SetColorMode(1);
  glyph->SetScaleMode(2);
  glyph->SetScaleFactor(.25);
  glyph->Update();

  vtkSmartPointer<vtkTubeFilter> tube =
    vtkSmartPointer<vtkTubeFilter>::New();
  tube->SetInputConnection(pdb->GetOutputPort());
  tube->SetNumberOfSides(6);
  tube->CappingOff();
  tube->SetRadius(0.2);
  tube->SetVaryRadius(0);
  tube->SetRadiusFactor(10);
  tube->Update();

  vtkSmartPointer< vtkPolyData > tubeMesh =
    vtkSmartPointer< vtkPolyData >::New();
  tubeMesh->ShallowCopy(tube->GetOutput());
  vtkIdType N = tubeMesh->GetNumberOfPoints();

  vtkUnsignedCharArray * rgb_colors
    = vtkUnsignedCharArray::SafeDownCast(
      tubeMesh->GetPointData()->GetArray("rgb_colors"));
  const unsigned char tuple[3] = {127,127,127};
  if ((rgb_colors != NULL) & (rgb_colors->GetNumberOfComponents() == 3))
    for (vtkIdType i = 0; i < N ; ++i)
      rgb_colors->SetTupleValue(i, tuple);

  vtkSmartPointer< vtkAppendPolyData > appendFilter =
    vtkSmartPointer< vtkAppendPolyData >::New();
  appendFilter->AddInputConnection(glyph->GetOutputPort());
  addInputData(appendFilter, tubeMesh);
  appendFilter->Update();

  polyData->ShallowCopy(appendFilter->GetOutput());
  return;
}

static void ConvertDataSetToSurface(
    vtkAlgorithmOutput * outputPort,
    vtkPolyData * polyData)
{
  vtkSmartPointer< vtkDataSetSurfaceFilter > dataSetSurfaceFilter =
    vtkSmartPointer< vtkDataSetSurfaceFilter >::New();
  dataSetSurfaceFilter->SetInputConnection(outputPort);
  dataSetSurfaceFilter->Update();
  polyData->ShallowCopy(dataSetSurfaceFilter->GetOutput());
}
static void ReadLegacyVTK(const char * file_name, vtkPolyData * polyData)
{
  vtkSmartPointer < vtkDataSetReader > reader =
    vtkSmartPointer < vtkDataSetReader >::New();
  reader->SetFileName(file_name);
  reader->Update();
  if (NULL != reader->GetPolyDataOutput())
    {
    polyData->ShallowCopy(reader->GetPolyDataOutput());
    }
  else if (NULL != reader->GetUnstructuredGridOutput())
    {
    ConvertDataSetToSurface(reader->GetOutputPort(), polyData);
    }
  else if (NULL != reader->GetStructuredPointsOutput())
    {
    ConvertDataSetToSurface(reader->GetOutputPort(), polyData);
    //std::cerr << "unsupported: vtkStructuredPoints\n";
    }
  else if (NULL != reader->GetStructuredGridOutput())
    {
    ConvertDataSetToSurface(reader->GetOutputPort(), polyData);
    //std::cerr << "unsupported: vtkStructuredGrid\n";
    }
  else if (NULL != reader->GetRectilinearGridOutput())
    {
    ConvertDataSetToSurface(reader->GetOutputPort(), polyData);
    //std::cerr << "unsupported: vtkRectilinearGrid\n";
    }
  else
    {
    std::cerr << "unsupported: ????????\n";
    }
}

int GetVTKStereoType(const QByteArray & stereoType) {
  if (stereoType == "CRYSTAL_EYES")
    {
    return (VTK_STEREO_CRYSTAL_EYES);
    }
  else if (stereoType == "RED_BLUE")
    {
    return (VTK_STEREO_RED_BLUE);
    }
  else if (stereoType == "INTERLACED")
    {
    return (VTK_STEREO_INTERLACED);
    }
  else if (stereoType == "LEFT")
    {
    return (VTK_STEREO_LEFT);
    }
  else if (stereoType == "RIGHT")
    {
    return (VTK_STEREO_RIGHT);
    }
  else if (stereoType == "DRESDEN")
    {
    return (VTK_STEREO_DRESDEN);
    }
  else if (stereoType == "ANAGLYPH")
    {
    return (VTK_STEREO_ANAGLYPH);
    }
  else if (stereoType == "CHECKERBOARD")
    {
    return (VTK_STEREO_CHECKERBOARD);
    }
  #ifdef VTK_STEREO_SPLITVIEWPORT_HORIZONTAL
  else if (stereoType == "SPLITVIEWPORT_HORIZONTAL")
    {
    return (VTK_STEREO_SPLITVIEWPORT_HORIZONTAL);
    }
  #endif
  else
    {
      return -1;
    }
}

VTKViewer::VTKViewer() :
  m_renderer(vtkSmartPointer < vtkRenderer >::New())
{
  vtkSmartPointer < vtkRenderWindow > renderWindow =
    vtkSmartPointer < vtkRenderWindow >::New();
  renderWindow->StereoCapableWindowOn();
  renderWindow->AddRenderer(m_renderer);
  QByteArray stereoType = qgetenv( "STEREO_TYPE" );
  int vtkStereoType = GetVTKStereoType(stereoType);
  if (vtkStereoType != -1)
    {
    renderWindow->SetStereoType(vtkStereoType);
    }
  else
    {
    renderWindow->SetStereoType(VTK_STEREO_RED_BLUE);
    }
  m_renderer->ResetCamera();
  this->SetRenderWindow(renderWindow);
  this->resize(480, 480);
  this->setMinimumSize(480, 360);
  connect(&(m_timer), SIGNAL(timeout()), this, SLOT(rotate()));
  m_timer.start(66);
}

void VTKViewer::add(vtkPolyData * polyData)
{
  double range[2];
  polyData->GetScalarRange(range);
  vtkSmartPointer< vtkColorTransferFunction > colorMap
    = vtkSmartPointer< vtkColorTransferFunction >::New();
  colorMap->SetColorSpaceToLab();
  colorMap->AddRGBPoint(range[0], 0.865, 0.865, 0.865);
  colorMap->AddRGBPoint(range[1], 0.706, 0.016, 0.150);
  colorMap->Build();

  vtkSmartPointer < vtkPolyDataMapper > mapper =
    vtkSmartPointer < vtkPolyDataMapper >::New();
  mapper->SetLookupTable(colorMap);
  if (polyData->GetPointData()->GetNormals() == NULL)
    {
    vtkSmartPointer< vtkPolyDataNormals > polyDataNormals
      = vtkSmartPointer< vtkPolyDataNormals >::New();
    setInputData(polyDataNormals, polyData);
    polyDataNormals->SetFeatureAngle(90.0);
    mapper->SetInputConnection(polyDataNormals->GetOutputPort());
    }
  else
    {
    setInputData(mapper, polyData);
    }
  vtkSmartPointer < vtkActor > actor =
    vtkSmartPointer < vtkActor >::New();
  actor->GetProperty()->SetPointSize(3);
  actor->SetMapper(mapper);
  m_renderer->AddActor(actor);
}
template <typename T>
static void read(const char * file_name, vtkPolyData * polyData)
{
    vtkSmartPointer< T > reader =
      vtkSmartPointer< T >::New();
    reader->SetFileName(file_name);
    reader->Update();
    polyData->ShallowCopy(reader->GetOutput());
}

void VTKViewer::add(const char * file_name)
{
  // TODO:  add logic for other file formats.
  vtkSmartPointer < vtkPolyData > polyData =
    vtkSmartPointer< vtkPolyData >::New();
  QString filename = QString::fromUtf8(file_name).toLower();
  if (filename.endsWith(".vtk"))
    {
    ReadLegacyVTK(file_name, polyData);
    }
  else if (filename.endsWith(".vtp"))
    {
    read< vtkXMLPolyDataReader >(file_name, polyData);
    }
  else if (filename.endsWith(".ply"))
    {
    read< vtkPLYReader >(file_name, polyData);
    }
  else if (filename.endsWith(".obj"))
    {
    read< vtkOBJReader >(file_name, polyData);
    }
  else if (filename.endsWith(".stl"))
    {
    read< vtkSTLReader >(file_name, polyData);
    }
  else if (filename.endsWith(".vtu"))
    {
    vtkSmartPointer< vtkXMLUnstructuredGridReader > reader =
      vtkSmartPointer< vtkXMLUnstructuredGridReader >::New();
    reader->SetFileName(file_name);
    reader->Update();
    ConvertDataSetToSurface(reader->GetOutputPort(), polyData);
    }
  else if (filename.endsWith(".pdb"))
    {
    ReadPDB(file_name, polyData);
    }
  else
    {
    std::cerr << file_name
      << ": BAD FILE NAME.  "
      "Should end in VTK, VTP, PLY, OBJ, STL, VTU, or PDB.\n";
    exit(1);
    return;
    }
  this->add(polyData);
}

void VTKViewer::toggleRotate()
{
  if (m_timer.isActive())
    m_timer.stop();
  else
    m_timer.start(33);
}

void VTKViewer::rotate()
{
  vtkCamera * camera = m_renderer->GetActiveCamera();
  assert(camera != NULL);
  camera->Azimuth(1);
  m_renderer->GetRenderWindow()->Render();
}

void VTKViewer::setStereoType(int vtkStereoType)
{
  vtkRenderWindow * rw = m_renderer->GetRenderWindow();
  assert(rw != NULL);
  rw->SetStereoType(vtkStereoType);
}

void VTKViewer::toggleStereo()
{
  vtkRenderWindow * rw = m_renderer->GetRenderWindow();
  assert(rw != NULL);
  rw->SetStereoRender(! rw->GetStereoRender());
  rw->Render();
}

void VTKViewer::nextStereoType()
{
  vtkRenderWindow * rw = m_renderer->GetRenderWindow();
  assert(rw != NULL);
  int type = rw->GetStereoType();
  type = (type % 9) + 1;
  this->setStereoType(type);
  switch(type)
    {
    case 1: cout << "VTK_STEREO_CRYSTAL_EYES\n"; break;
    case 2: cout << "VTK_STEREO_RED_BLUE\n"; break;
    case 3: cout << "VTK_STEREO_INTERLACED\n"; break;
    case 4: cout << "VTK_STEREO_LEFT\n"; break;
    case 5: cout << "VTK_STEREO_RIGHT\n"; break;
    case 6: cout << "VTK_STEREO_DRESDEN\n"; break;
    case 7: cout << "VTK_STEREO_ANAGLYPH\n"; break;
    case 8: cout << "VTK_STEREO_CHECKERBOARD\n"; break;
    case 9: cout << "VTK_STEREO_SPLITVIEWPORT_HORIZONTAL\n"; break;
    }
  rw->Render();
}

