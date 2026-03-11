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

    def forward(self, x):
        x = F.relu(self.fc1(x))
        x = F.relu(self.fc2(x))
        x = F.relu(self.fc3(x))
        
        # We use LogSoftmax for numerical stability during training
        policy = F.log_softmax(self.policy_head(x), dim=-1)
        value = torch.tanh(self.value_head(x))
        
        return policy, value

    def predict(self, state_tensor):
        """Helper for C++ MCTS to call"""
        self.eval()
        with torch.no_grad():
            t = torch.FloatTensor(state_tensor).unsqueeze(0)
            log_p, v = self.forward(t)
            return torch.exp(log_p).cpu().numpy()[0], v.item()

