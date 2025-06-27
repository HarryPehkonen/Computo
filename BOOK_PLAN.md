# Book Planning Questions

## Questions for Refinement

### 1. Target Audience
Are you thinking beginners to programming, or experienced developers new to these tools?

I think experienced developers who are not familiar with Lisp, Scheme, etc.  Let's assume they know map, reduce, filter, and JSON, at least superficially.

### 2. Use Case Focus
Do you have specific domains in mind? (Web APIs, data science, DevOps, etc.)

No specific domain or use case.  It might be handy to pick specific use cases to demonstrate some capabilities that would be a good match for assorted domains.

### 3. Hands-On Style
Should every chapter have practical exercises, or more reference-style explanations?

I think just reference-style explanations.  Whitespace should be used to ensure the code is easy to read as well as to write.

### 4. Length & Depth
Are you envisioning a comprehensive 300+ page guide, or a focused 150-page practical manual?

A focused 150page practical manual.  For example, if I want to know how to use map, I should be able to go to the section and be able to figure it out.  There should also be pointers to other functionalities that go together with map, such as reduce.  I'm thinking "advanced examples", and then "please see also" section.

### 5. Real Examples
Would you prefer fictional examples for clarity, or real-world anonymized use cases?

The sections that show the main functionality of functions should be concise, and then advanced examples can be short real-world use cases.  A how-to-use-for section, perhaps.

### 6. Prerequisites
Should we assume JSON knowledge, or start from basics?

Let's assume JSON knowledge.

## Current Structure Notes

The progression moves from simple concepts → building blocks → integration → advanced topics, with each chapter building on previous knowledge while introducing practical patterns people actually need.

That's excellent!  Maybe what I specified above was more for the reference section.

## Potential Additions

- **Video/Interactive**: Should we plan for companion materials?  No video at this time.
- **Code Repository**: Examples and exercises in a separate repo?  Yes!  Excellent idea!
- **Community**: Discussion forums or contribution guidelines?  Perhaps GitHub for discussion forum.  Contribution guidelines are a good idea.  I would like to warn that I want the code to remain "clean".
- **Versioning**: How to handle updates as Computo/Permuto evolve?  After experimenting, I'll start numbering versions semantically, from v1.0.0 up.  Breaking-changes.additional-functionality.bug-fixes.