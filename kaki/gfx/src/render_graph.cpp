//
// Created by felix on 29/01/2022.
//

#include <cassert>
#include "render_graph.h"

kaki::RenderGraphBuilder::Pass &kaki::RenderGraphBuilder::Pass::color(uint32_t image) {
    colorAttachments.push_back(Attachment{
            .image = image,
            .clear = {},
    });
    return *this;
}

kaki::RenderGraphBuilder::Pass &
kaki::RenderGraphBuilder::Pass::colorClear(uint32_t image, const VkClearColorValue &clearValue) {
    colorAttachments.push_back(Attachment{
            .image = image,
            .clear = VkClearValue {.color = clearValue},
    });
    return *this;
}

kaki::RenderGraphBuilder::Pass &kaki::RenderGraphBuilder::Pass::depth(uint32_t image) {
    assert(!depthAttachment.has_value());
    depthAttachment = Attachment{
        .image = image,
        .clear = {},
    };
    return *this;
}

kaki::RenderGraphBuilder::Pass &
kaki::RenderGraphBuilder::Pass::depthClear(uint32_t image, const VkClearDepthStencilValue &clearValue) {
    assert(!depthAttachment.has_value());
    depthAttachment = Attachment{
            .image = image,
            .clear = VkClearValue{.depthStencil = clearValue},
    };
    return *this;
}