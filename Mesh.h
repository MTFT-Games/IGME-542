#pragma once

#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include <d3d12.h>
#include "vertex.h"

// Mesh object containing geometry data
class Mesh
{
public:
	Mesh(
		Vertex* vertexData,                                 // Vertex data of the mesh (array)
		unsigned int vertexCount,                           // Number of vertexes in the vertexData
		unsigned int* indexData,                            // List of indexes (indices?) into the vertex data to use
		unsigned int indexCount);                           // Number of indexes (indices?) in indexData
	Mesh(const wchar_t* fileName);
	
	~Mesh();


	// Get the vertex buffer pointer
	Microsoft::WRL::ComPtr<ID3D12Resource> GetVertexBuffer();
	// Get the index buffer pointer
	Microsoft::WRL::ComPtr<ID3D12Resource> GetIndexBuffer();
	D3D12_VERTEX_BUFFER_VIEW GetvbView();
	D3D12_INDEX_BUFFER_VIEW GetibView();
	// Get the number of indexes (indices?) in the index buffer
	unsigned int GetIndexCount();

	// Draw this mesh
	//void Draw();


private:
	// Buffers to hold actual geometry data
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;

	// Buffer views
	D3D12_VERTEX_BUFFER_VIEW vbView;
	D3D12_INDEX_BUFFER_VIEW ibView;

	// Number of indexes (or indices?) in the index buffer
	unsigned int indexCount;

	void CalculateTangents(Vertex* verts, int numVerts, unsigned int* indices, int numIndices);
};

