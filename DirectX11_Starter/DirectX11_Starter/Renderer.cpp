#include "Renderer.h"

std::vector<GameObject*> Renderer::registeredGOs = std::vector<GameObject*>();

Renderer::Renderer(){

};

Renderer::~Renderer(){

};

ID3D11Buffer* instanceBuffer = nullptr;
InstanceType* instances;
D3D11_BUFFER_DESC instanceBufferDesc;
D3D11_SUBRESOURCE_DATA instanceData;

void Renderer::Draw(){
	// Get the current device context
	ID3D11DeviceContext* deviceContext = DeviceManager::GetCurrentDeviceContext();
	ID3D11Device* device = DeviceManager::GetCurrentDevice();

	GameObject** renderList = new GameObject*[registeredGOs.size()];
	UINT renderCount = 0;

	Material* currentRenderMaterial = nullptr;
	// Get all gameobjects with a certain material and draw them
	for(std::list<Material*>::iterator itr = Material::_materials.begin();
			itr != Material::_materials.end(); itr++){

		// Get the first material from the render list
		currentRenderMaterial = *itr;

		// Set the proper input options for this material
		(*itr)->SetConstantBufferData(
				Transform::Identity().ModelMatrix(), // Model matrix of an identity transform
				Camera::MainCamera.GetViewMatrix(),
				Camera::MainCamera.GetProjectionMatrix());
		(*itr)->SetInputAssemblerOptions();

		// Loop through the open list and get all of the objects with the 
		// same material that we should add to the render list.
		for(int i = 0; i < registeredGOs.size(); i++){
			if(registeredGOs[i]->material == currentRenderMaterial){
				renderList[renderCount] = registeredGOs[i];
				renderCount++;
			}
		}
		if(renderCount == 0)
			continue;

		// Allocate memory for all of the instance data
		instances = new InstanceType[renderCount];

		// Loop through all render items and put them into the instance array
		for (int i = 0; i < renderCount; i++) {
			instances[i].position = renderList[i]->transform.position;
		}

		// Set up the description of the instance buffer.
		instanceBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		instanceBufferDesc.ByteWidth = sizeof(InstanceType) * renderCount;
		instanceBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		instanceBufferDesc.CPUAccessFlags = 0;
		instanceBufferDesc.MiscFlags = 0;
		instanceBufferDesc.StructureByteStride = 0;

		// Give the subresource structure a pointer to the instance data.
		instanceData.pSysMem = instances;
		instanceData.SysMemPitch = 0;
		instanceData.SysMemSlicePitch = 0;

		// Create an instance buffer for the instance data
		HR(device->CreateBuffer(&instanceBufferDesc, &instanceData, &instanceBuffer));

		// Set buffers in the input assembler
		UINT strides[2];
		UINT offsets[2];
		ID3D11Buffer* bufferPointers[2];

		strides[0] = Vertex::VertexSize(renderList[0]->mesh->VertexType());
		strides[1] = sizeof(InstanceType);

		offsets[0] = 0;
		offsets[1] = 0;

		bufferPointers[0] = renderList[0]->mesh->VertexBuffer();	
		bufferPointers[1] = instanceBuffer;

		// Set the current vertex buffer
		deviceContext->IASetVertexBuffers(0, 2, bufferPointers, strides, offsets);
		// Set the current index buffer
		deviceContext->IASetIndexBuffer(renderList[0]->mesh->IndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
		// Set the topology
		deviceContext->IASetPrimitiveTopology(renderList[0]->mesh->Topology());
	
		deviceContext->DrawIndexedInstanced(
			renderList[0]->mesh->IndexCount(),	// Index count per instance
			renderCount,						// # instances to render
			0,									// Start index location
			0,									// Start vertex location
			0);									// When to start using instances

		/* Draw individual model
		deviceContext->DrawIndexed(
			go->mesh->IndexCount(),	// The number of indices we're using in this draw
			0,
			0); */

		ReleaseMacro(instanceBuffer);
		delete[] instances;
		instances = nullptr;
		currentRenderMaterial = nullptr;
		renderCount = 0;
	}

	delete[] renderList;
};

void Renderer::RegisterGameObject(GameObject* go){
	registeredGOs.push_back(go);
};

void Renderer::UnRegisterGameObject(GameObject* go){
	registeredGOs.erase(std::remove(registeredGOs.begin(), registeredGOs.end(), go), registeredGOs.end());
};
