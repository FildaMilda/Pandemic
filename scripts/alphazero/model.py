import torch
import torch.nn as nn
import torch.nn.functional as F
import numpy as np

class PandemicNet(nn.Module):
    def __init__(self, input_size=682, action_size=1126):
        super(PandemicNet, self).__init__()
        
        # Shared Backbone
        self.fc1 = nn.Linear(input_size, 512)
        self.fc2 = nn.Linear(512, 512)
        self.fc3 = nn.Linear(512, 256)
        
        # Policy Head (Probability of each move)
        self.policy_head = nn.Linear(256, action_size)
        
        # Value Head (Expected outcome -1 to 1)
        self.value_head = nn.Linear(256, 1)

    def forward(self, x, valid_mask=None):
        x = F.relu(self.fc1(x))
        x = F.relu(self.fc2(x))
        x = F.relu(self.fc3(x))
        
        # 1. Get raw logits (unnormalized scores)
        policy_logits = self.policy_head(x)
        
        # 2. Apply Masking
        if valid_mask is not None:
            # valid_mask should be a boolean tensor where True = valid, False = invalid.
            # We use ~valid_mask to select the invalid positions and fill them with -1e9.
            # -1e9 is practically negative infinity, so exp(-1e9) will be 0 in softmax.
            policy_logits = policy_logits.masked_fill(~valid_mask, -1e9)
        
        # 3. Softmax
        # We use LogSoftmax for numerical stability during training
        policy = F.log_softmax(policy_logits, dim=-1)
        value = torch.tanh(self.value_head(x))
        
        return policy, value

    def predict(self, state_tensor, mask_array=None):
        """Helper for C++ MCTS to call"""
        self.eval()
        with torch.no_grad():
            t = torch.FloatTensor(state_tensor).unsqueeze(0)
            
            if mask_array is not None:
                # Convert numpy mask to boolean tensor
                m = torch.BoolTensor(mask_array).unsqueeze(0)
            else:
                m = None
                
            log_p, v = self.forward(t, valid_mask=m)
            
            # Convert log probabilities back to standard 0-1 probabilities
            return torch.exp(log_p).cpu().numpy()[0], v.item()