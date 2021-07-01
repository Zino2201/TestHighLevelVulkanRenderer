#pragma once

#include "Texture.hpp"

namespace cb::gfx
{

/** Render pass/framebuffer */
enum class AttachmentLoadOp
{
	Load,
	Clear,
	DontCare
};

enum class AttachmentStoreOp
{
	Store,
	DontCare
};

/** Description of an attachment */
struct AttachmentDescription
{
	Format format;
	SampleCountFlagBits samples;
	AttachmentLoadOp load_op;
	AttachmentStoreOp store_op;
	AttachmentLoadOp stencil_load_op;
	AttachmentStoreOp stencil_store_op;
	TextureLayout initial_layout;
	TextureLayout final_layout;

	AttachmentDescription(const Format& in_format,
		const SampleCountFlagBits& in_samples,
		const AttachmentLoadOp& in_load_op,
		const AttachmentStoreOp& in_store_op,
		const AttachmentLoadOp& in_stencil_load_op,
		const AttachmentStoreOp& in_stencil_store_op,
		const TextureLayout& in_initial_layout,
		const TextureLayout& in_final_layout) : format(in_format),
		samples(in_samples), load_op(in_load_op), store_op(in_store_op),
		stencil_load_op(in_stencil_load_op), stencil_store_op(in_stencil_store_op),
		initial_layout(in_initial_layout), final_layout(in_final_layout) {}

	bool operator==(const AttachmentDescription& in_desc) const
	{
		return format == in_desc.format &&
			samples == in_desc.samples &&
			load_op == in_desc.load_op &&
			store_op == in_desc.store_op &&
			stencil_load_op == in_desc.stencil_load_op &&
			stencil_store_op == in_desc.stencil_store_op &&
			initial_layout == in_desc.initial_layout &&
			final_layout == in_desc.final_layout;
	}
};

/**
 * Reference to an attachment
 */
struct AttachmentReference
{
	static constexpr uint32_t unused_attachment = ~0Ui32;

	/** Index of the attachmenet (mirror RenderPassCreateInfo::attachments) */
	uint32_t attachment;

	/** Attachment layout during the subpass */
	TextureLayout layout;

	AttachmentReference() : attachment(unused_attachment), layout(TextureLayout::Undefined) {}

	AttachmentReference(const uint32_t in_attachment,
		const TextureLayout in_layout) : attachment(in_attachment), layout(in_layout) {}

	bool operator==(const AttachmentReference& in_ref) const
	{
		return attachment == in_ref.attachment &&
			layout == in_ref.layout;
	}
};

struct SubpassDescription
{
	std::vector<AttachmentReference> input_attachments;
	std::vector<AttachmentReference> color_attachments;
	std::vector<AttachmentReference> resolve_attachments;
	AttachmentReference depth_stencil_attachment;
	std::vector<uint32_t> preserve_attachments;

	SubpassDescription(const std::vector<AttachmentReference>& in_input_attachments = {},
		const std::vector<AttachmentReference>& in_color_attachments = {},
		const std::vector<AttachmentReference>& in_resolve_attachments = {},
		const AttachmentReference& in_depth_stencil_attachment = AttachmentReference(),
		const std::vector<uint32_t>& in_preserve_attachments = {}) :
		input_attachments(in_input_attachments), color_attachments(in_color_attachments),
		resolve_attachments(in_resolve_attachments), depth_stencil_attachment(in_depth_stencil_attachment),
		preserve_attachments(in_preserve_attachments) {}

	bool operator==(const SubpassDescription& in_desc) const
	{
		return input_attachments == in_desc.input_attachments &&
			color_attachments == in_desc.color_attachments &&
			resolve_attachments == in_desc.resolve_attachments &&
			depth_stencil_attachment == in_desc.depth_stencil_attachment &&
			preserve_attachments == in_desc.preserve_attachments;
	}
};

struct RenderPassCreateInfo
{
	std::vector<AttachmentDescription> attachments;
	std::vector<SubpassDescription> subpasses;

	RenderPassCreateInfo(const std::vector<AttachmentDescription>& in_attachments,
		const std::vector<SubpassDescription>& in_subpasses) : attachments(in_attachments),
		subpasses(in_subpasses) {}

	bool operator==(const RenderPassCreateInfo& in_info) const
	{
		return attachments == in_info.attachments &&
			subpasses == in_info.subpasses;
	}
};

union ClearColorValue
{
	std::array<float, 4> float32;

	ClearColorValue(const std::array<float, 4>& in_float) : float32(in_float) {}
};

struct ClearDepthStencilValue
{
	float depth;
	uint32_t stencil;

	ClearDepthStencilValue(const float in_depth,
		const uint32_t in_stencil) : depth(in_depth), stencil(in_stencil) {}
};

union ClearValue
{
	ClearColorValue color;
	ClearDepthStencilValue depth_stencil;

	ClearValue(const ClearColorValue& in_color) :
		color(in_color) {}

	ClearValue(const ClearDepthStencilValue& in_depth_stencil) :
		depth_stencil(in_depth_stencil) {}
};

/**
 * A framebuffer (containers of texture views)
 */
struct Framebuffer
{
	std::array<BackendDeviceResource, max_attachments_per_framebuffer> color_attachments;
	std::array<BackendDeviceResource, max_attachments_per_framebuffer> depth_attachments;
	uint32_t width;
	uint32_t height;
	uint32_t layers;

	Framebuffer() : width(0), height(0), layers(1) {}

	bool operator==(const Framebuffer& other) const
	{
		return color_attachments == other.color_attachments &&
			depth_attachments == other.depth_attachments &&
			width == other.width &&
			height == other.height;
	}
};
	
}