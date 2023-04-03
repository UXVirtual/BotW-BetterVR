#pragma once

class RND_D3D12 {
    friend class RND_Renderer;

public:
    RND_D3D12();
    ~RND_D3D12();

    ID3D12Device* GetDevice() { return m_device.Get(); };

    ID3D12CommandQueue* GetCommandQueue() { return m_queue.Get(); };

    // todo: extract most to a base pipeline class if other pipelines are needed
    class PresentPipeline {
        friend class Texture;

    public:
        PresentPipeline();
        ~PresentPipeline();

        void BindAttachment(uint32_t attachmentIdx, ID3D12Resource* srcTexture, DXGI_FORMAT overwriteFormat = DXGI_FORMAT_UNKNOWN);
        void BindTarget(uint32_t targetIdx, ID3D12Resource* dstTexture, DXGI_FORMAT overwriteFormat = DXGI_FORMAT_UNKNOWN);
        void Render(ID3D12GraphicsCommandList* commandList, ID3D12Resource* swapchain);

    private:
        void RecreatePipeline(uint32_t targetIdx, DXGI_FORMAT targetFormat);

        ComPtr<ID3DBlob> m_vertexShader;
        ComPtr<ID3DBlob> m_pixelShader;

        ComPtr<ID3D12Resource> m_screenIndicesBuffer;
        D3D12_INDEX_BUFFER_VIEW m_screenIndicesView = {};

        ComPtr<ID3D12RootSignature> m_signature;
        ComPtr<ID3D12PipelineState> m_pipelineState;

        std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 1> m_attachmentHandles = {};
        std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 1> m_targetHandles = {};
        ComPtr<ID3D12DescriptorHeap> m_attachmentHeap;
        ComPtr<ID3D12DescriptorHeap> m_targetHeap;
        std::array<DXGI_FORMAT, 1> m_targetFormats = { DXGI_FORMAT_UNKNOWN };
    };

    template <bool blockTillExecuted>
    class CommandContext {
    public:
        template <typename F>
        CommandContext(ID3D12Device* d3d12Device, ID3D12CommandQueue* d3d12Queue, F&& recordCallback): m_device(d3d12Device), m_queue(d3d12Queue) {
            // Create commands to upload buffers
            m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&this->commmandAllocator));
            m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, this->commmandAllocator.Get(), nullptr, IID_PPV_ARGS(&this->commandList));

            recordCallback(this->commandList.Get());
        }

        ~CommandContext() {
            // Close command list and then execute command list in queue
            checkHResult(this->commandList->Close(), "Failed to close D3D12_CommandContext's queue");
            ID3D12CommandList* collectedList[] = { this->commandList.Get() };
            m_queue->ExecuteCommandLists((UINT)std::size(collectedList), collectedList);

            // If enabled, wait until the command list and the fence signal has been executed
            if constexpr (blockTillExecuted) {
                m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&this->blockFence));
                m_queue->Signal(this->blockFence.Get(), 1);

                HANDLE waitEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);
                checkAssert(waitEvent != NULL, "Failed to create upload event!");

                if (this->blockFence->GetCompletedValue() < 1) {
                    this->blockFence->SetEventOnCompletion(1, waitEvent);
                    WaitForSingleObject(waitEvent, INFINITE);
                }
                CloseHandle(waitEvent);
            }

            this->commmandAllocator.Reset();
        }

    private:
        ID3D12Device* m_device;
        ID3D12CommandQueue* m_queue;

        ComPtr<ID3D12CommandAllocator> commmandAllocator;
        ComPtr<ID3D12GraphicsCommandList> commandList;
        ComPtr<ID3D12Fence> blockFence;
    };

private:
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12CommandQueue> m_queue;
    ComPtr<ID3D12CommandAllocator> m_allocator;
    ComPtr<ID3D12GraphicsCommandList> m_list;

    std::vector<std::unique_ptr<PresentPipeline>> m_pipelines;

    //ComPtr<ID3D12Fence> m_fence;
    //HANDLE m_fenceEvent;
    //uint64_t m_fenceValue;
};